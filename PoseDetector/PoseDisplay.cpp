#include "PoseDetector.h"
#include "CvTestbed.h"
#include "MarkerDetector.h"
#include "Shared.h"
using namespace alvar;
using namespace std;

bool init = true;
const int marker_size = 15;
Camera cam;
stringstream calibrationFilename;

PoseDetector::PoseDetector()
{}

void videocallback(IplImage* image)
{
    static IplImage* rgba;
    bool flip_image = (image->origin ? true : false);
    if (flip_image) {
        cvFlip(image);
        image->origin = !image->origin;
    }

    if (init) {
        init = false;
        cout << "Loading calibration: " << calibrationFilename.str();
        if (cam.SetCalib(calibrationFilename.str().c_str(), image->width, image->height)) {
            cout << " [Ok]" << endl;
        }
        else {
            cam.SetRes(image->width, image->height);
            cout << " [Fail]" << endl;
        }
        double p[16];
        cam.GetOpenglProjectionMatrix(p, image->width, image->height);

        rgba = CvTestbed::Instance().CreateImageWithProto("RGBA", image, 0, 4);
    }

    static MarkerDetector<MarkerData> marker_detector;
    marker_detector.SetMarkerSize(marker_size);
    marker_detector.Detect(image, &cam, true, true);

    for (size_t i = 0; i < marker_detector.markers->size(); i++) {
        if (i >= 32) break;

        Pose p = (*(marker_detector.markers))[i].pose;
        p.Output(); //Output Pose

        //CREATE FOTMATED DISPLAY
        
    }

    if (flip_image) {
        cvFlip(image);
        image->origin = !image->origin;
    }
}

void PoseDetector::PoseOutput(int argc, char* argv[])
{
    try {
        // Output usage message
        string filename(argv[0]);
        filename = filename.substr(filename.find_last_of('\\') + 1);

        // Initialise GlutViewer and CvTestbed
        CvTestbed::Instance().SetVideoCallback(videocallback);

        // Create capture object from camera (argv[1] is a number) or from file (argv[1] is a string)
        Capture* cap;
        string uniqueName;
        if ((argc > 1) && (!isdigit(argv[1][0]))) {
            // Manually create capture device and initialize capture object
            CaptureDevice device("file", argv[1]);
            cap = CaptureFactory::instance()->createCapture(device);
            uniqueName = "file";
        }
        else {
            // Enumerate possible capture plugins
            CaptureFactory::CapturePluginVector plugins = CaptureFactory::instance()->enumeratePlugins();
            if (plugins.size() < 1) {
                cout << "Could not find any capture plugins." << endl;
                return;
            }

            // Display capture plugins
            cout << "Available Plugins: ";
            outputEnumeratedPlugins(plugins);
            cout << endl;

            // Enumerate possible capture devices
            CaptureFactory::CaptureDeviceVector devices = CaptureFactory::instance()->enumerateDevices();
            if (devices.size() < 1) {
                cout << "Could not find any capture devices." << endl;
                return;
            }

            // Check command line argument for which device to use
            int selectedDevice = defaultDevice(devices);
            if (argc > 1) {
                selectedDevice = atoi(argv[1]);
            }
            if (selectedDevice >= (int)devices.size()) {
                selectedDevice = defaultDevice(devices);
            }

            // Display capture devices
            cout << "Enumerated Capture Devices:" << endl;
            outputEnumeratedDevices(devices, selectedDevice);
            cout << endl;

            // Create capture object from camera
            cap = CaptureFactory::instance()->createCapture(devices[selectedDevice]);
            uniqueName = devices[selectedDevice].uniqueName();
        }

        // Handle capture lifecycle and start video capture
        // Note that loadSettings/saveSettings are not supported by all plugins
        if (cap) {
            stringstream settingsFilename;
            settingsFilename << "camera_settings_" << uniqueName << ".xml";
            calibrationFilename << "camera_calibration_" << uniqueName << ".xml";

            cap->start();
            cap->setResolution(640, 480);

            if (cap->loadSettings(settingsFilename.str())) {
                cout << "Loading settings: " << settingsFilename.str() << endl;
            }

            stringstream title;
            title << "SampleMarkerDetector (" << cap->captureDevice().captureType() << ")";

            CvTestbed::Instance().StartVideo(cap, title.str().c_str());

            if (cap->saveSettings(settingsFilename.str())) {
                cout << "Saving settings: " << settingsFilename.str() << endl;
            }

            cap->stop();
            delete cap;
        }
        else if (CvTestbed::Instance().StartVideo(0, argv[0])) {
        }
        else {
            cout << "Could not initialize the selected capture backend." << endl;
        }
        
        return;
    }
    catch (const exception& e) {
        cout << "Exception: " << e.what() << endl;
    }
    catch (...) {
        cout << "Exception: unknown" << endl;
    }
}
