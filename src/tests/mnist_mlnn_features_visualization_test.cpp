/*!
 * @file mnist_mlnn_features_visualization_test.cpp
 * @brief Program for visualization of features of mlnn layer trained on MNIST digits.
 * @author tkornuta
 * @date:   03-04-2017
 *
 * Copyright (c) 2017, Tomasz Kornuta, IBM Corporation. All rights reserved.
 *
 */

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <data_io/MNISTMatrixImporter.hpp>

#include <logger/Log.hpp>
#include <logger/ConsoleOutput.hpp>
using namespace mic::logger;

#include <application/ApplicationState.hpp>

#include <configuration/ParameterServer.hpp>

#include <opengl/visualization/WindowManager.hpp>
#include <opengl/visualization/WindowGrayscaleBatch.hpp>
#include <opengl/visualization/WindowCollectorChart.hpp>
using namespace mic::opengl::visualization;

// Neural net.
#include <mlnn/BackpropagationNeuralNetwork.hpp>
using namespace mic::mlnn;

// Encoders.
#include <encoders/MatrixXfMatrixXfEncoder.hpp>
#include <encoders/UIntMatrixXfEncoder.hpp>

/// Windows for displaying activations.
WindowGrayscaleBatch<float> *w_conv10, *w_conv11, *w_conv12, *w_conv13, *w_conv14, *w_conv15;
WindowGrayscaleBatch<float> *w_conv20, *w_conv21, *w_conv22, *w_conv23, *w_conv24, *w_conv25;
WindowGrayscaleBatch<float> *w_conv30, *w_conv31, *w_conv32, *w_conv33, *w_conv34, *w_conv35;
/// Window for displaying chart with statistics.
WindowCollectorChart<float>* w_chart;
/// Data collector .
mic::data_io::DataCollectorPtr<std::string, float> collector_ptr;


/// MNIST importer.
mic::data_io::MNISTMatrixImporter* importer;
/// Multi-layer neural network.
BackpropagationNeuralNetwork<float> neural_net;

/// MNIST matrix encoder.
mic::encoders::MatrixXfMatrixXfEncoder* mnist_encoder;
/// Label 2 matrix encoder (1 hot).
mic::encoders::UIntMatrixXfEncoder* label_encoder;

const size_t batch_size = 1;
const char* fileName = "nn_autoencoder_weights_visualization.txt";


/*!
 * \brief Function for batch sampling.
 * \author tkornuta
 */
void batch_function (void) {

/*	if (neural_net.load(fileName)) {
		LOG(LINFO) << "Loaded neural network from a file";
	} else {*/
		{
//			neural_net.pushLayer(new mic::mlnn::convolution::Padding<float>(24, 24, 1, 1));

			neural_net.pushLayer(new mic::mlnn::convolution::Cropping<float>(28, 28, 1, 2));
	/*		neural_net.pushLayer(new mic::mlnn::convolution::Convolution<float>(24, 24, 1, 9, 5, 1));
			neural_net.pushLayer(new ELU<float>(20, 20, 9));
			neural_net.pushLayer(new mic::mlnn::convolution::MaxPooling<float>(20, 20, 9, 2));

			neural_net.pushLayer(new mic::mlnn::convolution::Convolution<float>(10, 10, 9, 16, 7, 1));
			neural_net.pushLayer(new ELU<float>(4, 4, 16));
			neural_net.pushLayer(new mic::mlnn::convolution::MaxPooling<float>(4, 4, 16, 2));

			neural_net.pushLayer(new Linear<float>(2, 2, 16, 10, 1, 1));
			neural_net.pushLayer(new Softmax<float>(10));*/

			neural_net.pushLayer(new Linear<float>(24, 24, 1, 10, 1, 1));
			neural_net.pushLayer(new Softmax<float>(10));

			if (!neural_net.verify())
				exit(-1);


			neural_net.setLoss<  mic::neural_nets::loss::SquaredErrorLoss<float> >();
			neural_net.setOptimization<  mic::neural_nets::optimization::Adam<float> >();

		LOG(LINFO) << "Generated new neural network";
	}//: else

	// Import data from datasets.
	if (!importer->importData())
		exit(-1);


	size_t iteration = 0;

	// Retrieve the next minibatch.
	//mic::types::MNISTBatch bt = importer->getNextBatch();
	//importer->setNextSampleIndex(5);

	// Main application loop.
	while (!APP_STATE->Quit()) {

		// If not paused.
		if (!APP_STATE->isPaused()) {

			// If single step mode - pause after the step.
			if (APP_STATE->isSingleStepModeOn())
				APP_STATE->pressPause();

			{ // Enter critical section - with the use of scoped lock from AppState!
				APP_DATA_SYNCHRONIZATION_SCOPED_LOCK();

				// Retrieve the next minibatch.
				mic::types::MNISTBatch bt = importer->getRandomBatch();

				// Encode data.
				mic::types::MatrixXfPtr encoded_batch = mnist_encoder->encodeBatch(bt.data());
				mic::types::MatrixXfPtr encoded_labels = label_encoder->encodeBatch(bt.labels());

/*				mic::types::MatrixPtr<float> encoded_batch = MAKE_MATRIX_PTR(float, patch_size*patch_size, 1);
				for (size_t i=0; i<patch_size*patch_size; i++)
					(*encoded_batch)[i]= 1.0 -(float)i/(patch_size*patch_size);*/
				/*mic::types::MatrixPtr<float> encoded_labels = MAKE_MATRIX_PTR(float, output_size, 1);
				encoded_labels->setZero();
				(*encoded_labels)[0]= 1.0;*/
				/*(*encoded_labels)[6]= 1.0;
				(*encoded_labels)[9]= 1.0;
				(*encoded_labels)[15]= 1.0;*/

				// Train the autoencoder.
				float loss = neural_net.train (encoded_batch, encoded_labels, 0.001, 0.0001);

				// Get reconstruction.
				/*mic::types::MatrixXfPtr encoded_reconstruction = neural_net.getPredictions();
				std::vector<mic::types::MatrixXfPtr> decoded_reconstruction = mnist_encoder->decodeBatch(encoded_reconstruction);
				w_reconstruction->setBatchUnsynchronized(decoded_reconstruction);*/

				if (iteration%10 == 0) {
					// Visualize the weights.
					//std::shared_ptr<Layer<float> > layer1 = neural_net.getLayer(3);

/*					std::shared_ptr<mic::mlnn::convolution::Convolution<float> > conv1 =
							neural_net.getLayer<mic::mlnn::convolution::Convolution<float> >(1);
					w_conv10->setBatchUnsynchronized(conv1->getInputActivations());
					w_conv11->setBatchUnsynchronized(conv1->getInputGradientActivations());
					w_conv12->setBatchUnsynchronized(conv1->getWeightActivations());
					w_conv13->setBatchUnsynchronized(conv1->getWeightGradientActivations());
					w_conv14->setBatchUnsynchronized(conv1->getOutputActivations());
					w_conv15->setBatchUnsynchronized(conv1->getOutputGradientActivations());

					std::shared_ptr<mic::mlnn::convolution::Convolution<float> > conv2 =
							neural_net.getLayer<mic::mlnn::convolution::Convolution<float> >(4);
					w_conv20->setBatchUnsynchronized(conv2->getInputActivations());
					w_conv21->setBatchUnsynchronized(conv2->getInputGradientActivations());
					w_conv22->setBatchUnsynchronized(conv2->getWeightActivations());
					w_conv23->setBatchUnsynchronized(conv2->getWeightGradientActivations());
					w_conv24->setBatchUnsynchronized(conv2->getOutputActivations());
					w_conv25->setBatchUnsynchronized(conv2->getOutputGradientActivations());

					std::shared_ptr<mic::mlnn::fully_connected::Linear<float> > lin1 =
							neural_net.getLayer<mic::mlnn::fully_connected::Linear<float> >(7);
					w_conv30->setBatchUnsynchronized(lin1->getInputActivations());
					w_conv31->setBatchUnsynchronized(lin1->getInputGradientActivations());
					w_conv32->setBatchUnsynchronized(lin1->getWeightActivations());
					w_conv33->setBatchUnsynchronized(lin1->getWeightGradientActivations());

					std::shared_ptr<Layer<float> > sm1 = neural_net.getLayer(7);
					w_conv34->setBatchUnsynchronized(sm1->getOutputActivations());
					w_conv35->setBatchUnsynchronized(sm1->getOutputGradientActivations());*/

					std::shared_ptr<mic::mlnn::fully_connected::Linear<float> > lin1 =
							neural_net.getLayer<mic::mlnn::fully_connected::Linear<float> >(1);
					w_conv10->setBatchUnsynchronized(lin1->getInputActivations());
					w_conv11->setBatchUnsynchronized(lin1->getInputGradientActivations());
					w_conv12->setBatchUnsynchronized(lin1->getWeightActivations());
					w_conv13->setBatchUnsynchronized(lin1->getWeightGradientActivations());
					w_conv14->setBatchUnsynchronized(lin1->getOutputActivations());
					w_conv15->setBatchUnsynchronized(lin1->getOutputGradientActivations());

					w_conv20->setBatchUnsynchronized(lin1->getInverseWeightActivations());
					w_conv21->setBatchUnsynchronized(lin1->getInverseOutputActivations());


				}//: if

				iteration++;
				LOG(LINFO) << "Iteration: " << iteration << " loss =" << loss ;
				// Add data to chart window.
				collector_ptr->addDataToContainer("Loss", loss);
			}//: end of critical section

		}//: if

		// Sleep.
		APP_SLEEP();
	}//: while

}//: image_encoder_and_visualization_test



/*!
 * \brief Main program function. Runs two threads: main (for GLUT) and another one (for data processing).
 * \author tkornuta
 * @param[in] argc Number of parameters (passed to glManaged).
 * @param[in] argv List of parameters (passed to glManaged).
 * @return (not used)
 */
int main(int argc, char* argv[]) {
	// Set console output to logger.
	LOGGER->addOutput(new ConsoleOutput());
	LOG(LINFO) << "Logger initialized. Starting application";

	// Parse parameters.
	PARAM_SERVER->parseApplicationParameters(argc, argv);

	// Initilize application state ("touch it") ;)
	APP_STATE;

	// Load dataset.
	importer = new mic::data_io::MNISTMatrixImporter();
	importer->setBatchSize(batch_size);

	// Initialize the encoders.
	mnist_encoder = new mic::encoders::MatrixXfMatrixXfEncoder(28, 28);
	label_encoder = new mic::encoders::UIntMatrixXfEncoder(10);

	// Set parameters of all property-tree derived objects - USER independent part.
	PARAM_SERVER->loadPropertiesFromConfiguration();

	// Initialize property-dependent variables of all registered property-tree objects - USER dependent part.
	PARAM_SERVER->initializePropertyDependentVariables();

	// Initialize GLUT! :]
	VGL_MANAGER->initializeGLUT(argc, argv);

	// Create batch visualization window.
	w_conv10 = new WindowGrayscaleBatch<float>("Conv1 x", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 50, 50, 256, 256);
	w_conv11 = new WindowGrayscaleBatch<float>("Conv1 dx", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 316, 50, 256, 256);
	w_conv12 = new WindowGrayscaleBatch<float>("Conv1 W", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 562, 50, 256, 256);
	w_conv13 = new WindowGrayscaleBatch<float>("Conv1 dW", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 818, 50, 256, 256);
	w_conv14 = new WindowGrayscaleBatch<float>("Conv1 y", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 1074, 50, 256, 256);
	w_conv15 = new WindowGrayscaleBatch<float>("Conv1 dy", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 1330, 50, 256, 256);

	w_conv20 = new WindowGrayscaleBatch<float>("Conv2 x", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 50, 336, 256, 256);
	w_conv21 = new WindowGrayscaleBatch<float>("Conv2 dx", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 316, 336, 256, 256);
	w_conv22 = new WindowGrayscaleBatch<float>("Conv2 W", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 562, 336, 256, 256);
	w_conv23 = new WindowGrayscaleBatch<float>("Conv2 dW", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 818, 336, 256, 256);
	w_conv24 = new WindowGrayscaleBatch<float>("Conv2 y", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 1074, 336, 256, 256);
	w_conv25 = new WindowGrayscaleBatch<float>("Conv2 dy", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 1330, 336, 256, 256);

	w_conv30 = new WindowGrayscaleBatch<float>("L1 x", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 50, 622, 256, 256);
	w_conv31 = new WindowGrayscaleBatch<float>("L1 dx", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 316, 622, 256, 256);
	w_conv32 = new WindowGrayscaleBatch<float>("L1 W", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 562, 622, 256, 256);
	w_conv33 = new WindowGrayscaleBatch<float>("L1 dW", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 818, 622, 256, 256);
	w_conv34 = new WindowGrayscaleBatch<float>("SM y", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 1074, 622, 256, 256);
	w_conv35 = new WindowGrayscaleBatch<float>("SM dy", WindowGrayscaleBatch<float>::Norm_HotCold, WindowGrayscaleBatch<float>::Grid_Both, 1330, 622, 256, 256);

	// Chart.
	w_chart = new WindowCollectorChart<float>("Statistics", 60, 60, 512, 256);
	collector_ptr= std::make_shared < mic::data_io::DataCollector<std::string, float> >( );
	w_chart->setDataCollectorPtr(collector_ptr);

	// Create data containers.
	collector_ptr->createContainer("Loss", mic::types::color_rgba(255, 0, 0, 180));
	//collector_ptr->createContainer("Correct classifications", mic::types::color_rgba(255, 255, 255, 180));

	boost::thread batch_thread(boost::bind(&batch_function));

	// Start visualization thread.
	VGL_MANAGER->startVisualizationLoop();

	LOG(LINFO) << "Waiting for threads to join...";
	// End test thread.
	batch_thread.join();
	LOG(LINFO) << "Threads joined - ending application";
}//: main
