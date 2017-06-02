// (C) Copyright Kirill Lykov 2013.
//
// Contributed author Vadim Frolov
// Distributed under the FreeBSD Software License (See accompanying file license.txt)

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#define OPENNI2
// OpenNI or OpenNI2
#ifdef OPENNI2
#include <OpenNI.h>
#define THROW_IF_FAILED(retVal) {if (retVal != openni::STATUS_OK) throw openni::OpenNI::getExtendedError();}
#else
#include <XnCppWrapper.h>
#define THROW_IF_FAILED(retVal) {if (retVal != XN_STATUS_OK) throw xnGetStatusString(retVal);}
#endif


// OpenCV
#include <opencv2/opencv.hpp>

/**
 * @class
 *  Helper class for tackling codec names
 */
class CodecName2FourCC
{
    typedef std::map<std::string, std::string> Map;
    typedef std::map<std::string, std::string>::iterator Iterator;
    Map m_codecName2FourCC;
    public:
    CodecName2FourCC()
    {
        m_codecName2FourCC["MPEG-1"] = "PIM1";
        m_codecName2FourCC["MPEG-4.2"] = "MP42";
        m_codecName2FourCC["MPEG-4.3"] = "DIV3";
        m_codecName2FourCC["MPEG-4"] = "DIVX";
        m_codecName2FourCC["FLV1"] = "FLV1";
    }

    int operator() (const std::string& codeName)
    {
        Iterator it = m_codecName2FourCC.find(codeName);
        if (it == m_codecName2FourCC.end())
            throw "unknown codec name";
        std::string fourcc = it->second;
        return CV_FOURCC(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
    }
};

std::ostream& operator << (std::ostream& stream, const OniVersion& item)
{
    stream << static_cast<int>(item.major) << "." << static_cast<int>(item.minor) << "." 
           << item.maintenance << ". Build " << item.build;
    return stream;
}

std::ostream& operator << (std::ostream& stream, openni::Device& device)
{
    //stream << "\tOpenNI version: " << item.Version << std::endl;
    auto sinfoDepth = device.getSensorInfo(openni::SENSOR_DEPTH);
    auto sinfoImg = device.getSensorInfo(openni::SENSOR_COLOR);
    const openni::DeviceInfo& item = device.getDeviceInfo();
    stream << "\tType: " << sinfoDepth->getSensorType() << ", " << sinfoImg->getSensorType() 
           << ". Generator name:  " << item.getName() << ". Vendor: " << item.getVendor() << "." << std::endl;
    return stream;
}

/**
 * @class
 *  Normalize colors in depth using histogram as proposed by user Vlad:
 *  http://stackoverflow.com/questions/17944590/convert-kinects-depth-to-rgb
 *  The original idea is from
 *  https://github.com/OpenNI/OpenNI2/blob/master/Samples/Common/OniSampleUtilities.h
 */
class HistogramNormalizer
{
    public:
        static void run(cv::Mat& input)
        {
            // TODO smth like cv::equalizeHist(depthMat8UC1, depth2);
            // should give the same result but is not. Check it out
            std::vector<float> histogram;
            calculateHistogram(input, histogram);
            cv::MatIterator_<short> it = input.begin<short>(), it_end = input.end<short>();
            for(; it != it_end; ++it) {
                *it = histogram[*it];
            }
        }

    private:
        static void calculateHistogram(const cv::Mat& depth, std::vector<float>& histogram)
        {
            int depthTypeSize = CV_ELEM_SIZE(depth.type());
            int histogramSize = pow(2., 8 * depthTypeSize);
            histogram.resize(histogramSize, 0.0f);

            unsigned int nNumberOfPoints = 0;
            cv::MatConstIterator_<short> it = depth.begin<short>(), it_end = depth.end<short>();
            for(; it != it_end; ++it) {
                if (*it != 0) {
                    ++histogram[*it];
                    ++nNumberOfPoints;
                }
            }

            for (int nIndex = 1; nIndex < histogramSize; ++nIndex)
            {
                histogram[nIndex] += histogram[nIndex - 1];
            }

            if (nNumberOfPoints != 0)
            {
                for (int nIndex = 1; nIndex < histogramSize; ++nIndex)
                {
                    histogram[nIndex] = (256.0 * (1.0f - (histogram[nIndex] / nNumberOfPoints)));
                }
            }
        }
};

/**
 * @class
 *  Convert oni file to avi or images
 */
class Oni2AviConverter
{
    CodecName2FourCC m_codecName2Code;
    public:
    Oni2AviConverter()
    {

    }

    void run(const std::string& codecName,
            const std::string& inputFile, const std::string& outputFile, bool depthAsPng = false)
    {
        // TODO For the latest version of OpenCV, you may use HighGUI instead of using OpenNI
        // assumed that nframes, picture size for depth and images is the same
        THROW_IF_FAILED( openni::OpenNI::initialize() );
        openni::Device device;
        THROW_IF_FAILED( device.open(inputFile.c_str()) );

        openni::VideoStream imageStream;
        if (device.getSensorInfo(openni::SENSOR_COLOR) == nullptr)
            throw "ERROR: getSensorInfo returned null for color";
        THROW_IF_FAILED( imageStream.create(device, openni::SENSOR_COLOR) );
        imageStream.start();

        openni::VideoMode vm = imageStream.getVideoMode();
        int fps = vm.getFps();
        int frame_height = vm.getResolutionY();
        int frame_width = vm.getResolutionX();

        if (vm.getPixelFormat() != openni::PIXEL_FORMAT_RGB888) {
            vm.setPixelFormat(openni::PIXEL_FORMAT_RGB888);
        }

        openni::VideoStream depthStream;        
        if (device.getSensorInfo(openni::SENSOR_DEPTH) == nullptr)
            throw "ERROR: getSensorInfo returned null for depth";
        THROW_IF_FAILED( depthStream.create(device, openni::SENSOR_DEPTH) );
        depthStream.start();

        if (!depthStream.isValid() || !imageStream.isValid())
            throw "ERROR: something went wrong with streams";

        auto playbackControl = device.getPlaybackControl();
        int nframes = playbackControl->getNumberOfFrames(depthStream);

        std::string outputFileImg, outputFileDepth;
        getOutputFileNames(outputFile, outputFileImg, outputFileDepth);

        printResume(nframes, codecName, inputFile, outputFileImg, depthStream);

        //check permissions to write in the current directory
        //fs::path currentFolder("./");
        //fs::file_status st = fs::status(currentFolder);
        //std::cout << (st.permissions() & fs::all_all) << std::endl;
        boost::filesystem::wpath file(outputFileImg); 
        if(boost::filesystem::exists(file)) 
            boost::filesystem::remove(file); 
        file = outputFileDepth;        
        if(!depthAsPng && boost::filesystem::exists(file)) 
            boost::filesystem::remove(file);

        cv::VideoWriter imgWriter(outputFileImg, m_codecName2Code(codecName), fps, cvSize(frame_width, frame_height), 1);
        if (!imgWriter.isOpened())
            std::cout << "Output video could not be opened" << std::endl;

        cv::VideoWriter depthWriter;
        if (!depthAsPng)
            depthWriter.open(outputFileDepth, m_codecName2Code(codecName), fps, cvSize(frame_width, frame_height), 1);

        fs::path folderForDepthImages = getDepthFolderName(outputFileImg);
        if (depthAsPng)
        {
            if (fs::exists(folderForDepthImages) && !fs::is_directory(folderForDepthImages))
            {
                throw "Cannot create a directory because file with the same name exists. Remove it and try again";
            }
            if (!fs::exists(folderForDepthImages))
            {
                fs::create_directory(folderForDepthImages);
            }
        }

        std::vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(0);

        size_t outStep = nframes > 10 ? nframes / 10 : 1;

        try    
        {
            int iframe = 0;
            while (iframe < nframes) 
            {
                if (playbackControl->seek(imageStream, iframe) != openni::STATUS_OK || playbackControl->seek(depthStream, iframe) != openni::STATUS_OK)
                    throw "Something went wrong while reading frame";
                if ( iframe % outStep == 0 )
                    std::cout << iframe << "/" << nframes << std::endl;

                // save image
                openni::VideoFrameRef frame;
                imageStream.readFrame(&frame);
                cv::Mat image(frame_height, frame_width, CV_8UC3, const_cast<void*>(frame.getData()));

                cv::cvtColor(image, image, CV_BGR2RGB); // opencv image format is BGR
                //Saving Image frames
                //std::ostringstream ss_img;
                //ss_img << "rgb-" << iframe << ".ppm";
                //std::string color_mat = ss_img.str();
                //cv::imwrite(color_mat,image);
                imgWriter << image.clone();

                // save depth
                depthStream.readFrame(&frame);
                cv::Mat depth(frame_height, frame_width, CV_16U, const_cast<void*>(frame.getData()));

                // Contributed by user alexmylonas: "Added functionality to extract high quality images and depth map"
                // saving depth frames
                //std::ostringstream ss_depth;
                //ss_depth << "depth-" << iframe;
                //std::string depth_mat = ss_depth.str();
                //cv::FileStorage file(depth_mat, cv::FileStorage::WRITE);
                //file << depth_mat << depth; 

                if (!depthAsPng)
                {
#if (CV_MAJOR_VERSION == 2 && CV_MINOR_VERSION >= 4) || CV_MAJOR_VERSION > 2
                    HistogramNormalizer::run(depth);
                    cv::Mat depthMat8UC1;
                    depth.convertTo(depthMat8UC1, CV_8UC1);

                    // can be used for having different colors than grey
                    cv::Mat falseColorsMap;
                    cv::applyColorMap(depthMat8UC1, falseColorsMap, cv::COLORMAP_AUTUMN);
                    depthWriter << falseColorsMap;
#else
                    throw "saving depth in avi file is not supported for opencv 2.3 or earlier. please use option --depth-png=yes";
#endif
                }
                else
                {
                    // to_string is not supported by gcc4.7 so I don't use it here
                    //std::string imgNumAsStr = std::to_string(imgNum);
                    std::stringstream ss;
                    ss << folderForDepthImages.string() << "/depth-" << iframe << ".png";
                    cv::imwrite(ss.str(), depth, compression_params);
                }
                ++iframe;
            }
        }
        catch(...)
        {
            imageStream.stop();
            imageStream.destroy();
            depthStream.stop();
            depthStream.destroy();
            device.close();
            openni::OpenNI::shutdown();
            throw;
        }
        imgWriter.release();
        imageStream.stop();
        imageStream.destroy();
        depthStream.stop();
        depthStream.destroy();
        device.close();
        openni::OpenNI::shutdown();
    }

    private:

    static void printResume(size_t nframes, const std::string& codecName,
            const std::string& inputFile, const std::string& outputFile, openni::VideoStream& depthGen)
    {
        std::cout <<
            "Input file name: " << inputFile << ".\nOutput file name: " << outputFile
            << ".\n\tTotal: " << nframes << " frames. Used codec: " << codecName << std::endl;

        // Could not find similar
        //xn::NodeInfo nin = depthGen.GetInfo();
        //if ((XnNodeInfo*)nin == 0)
        //    throw "Could not read DepthGenerator info. Probably, the input file is corrupted";
        //XnProductionNodeDescription description = nin.GetDescription();
        //std::cout << description;
    }

    static void getOutputFileNames(const std::string& outputFileName, std::string& outputFileImg,
            std::string& outputFileDepth)
    {
        fs::path outPath(outputFileName);
        if (outPath.extension() != ".avi")
            throw "output file extention must be avi";

        std::string nameWithoutExtension = (outPath.parent_path() / outPath.stem()).string();

        outputFileImg = nameWithoutExtension + "-img.avi";
        outputFileDepth = nameWithoutExtension + "-depth.avi";
    }

    static fs::path getDepthFolderName(fs::path outPath)
    {
        fs::path fnNoExt = outPath.stem();
        //fnNoExt += "-depth"; cannot use it because boost 1.48 doesn't have operator +=
        return fs::path(fnNoExt.string() + "-depth");
    }

    Oni2AviConverter(const Oni2AviConverter&);
    Oni2AviConverter& operator= (const Oni2AviConverter&);
};

int main(int argc, char* argv[])
{
    try
    {
        po::options_description desc("oni2avi converts an input oni file into 2 avi files - one for image and another for the depth map."
                "\n Allowed options:");
        desc.add_options()
            ("help", "produce help message")
            ("codec", po::value<std::string>()->default_value("MPEG-4.2"),
             "codec used for output video. Available codecs are MPEG-1, MPEG-4, MPEG-4.2, MPEG-4.3 , FLV1")
            ("input-file", po::value< std::string >(), "input oni file")
            ("output-file", po::value< std::string >(), "output avi file")
            ("depth-png", po::value< std::string >(), "save depth as png images instead of avi file. Available values yes or no")
            ;

        po::positional_options_description p;
        p.add("input-file", 1);
        p.add("output-file", 2);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                options(desc).positional(p).run(), vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 1;
        }

        if (!vm.count("codec"))
            throw "codec was not set\n";

        if (!vm.count("input-file"))
            throw "input file was not set\n";

        if (!vm.count("output-file"))
            throw "output file was not set\n";

        bool depthAsPng = false;
        if (vm.count("depth-png"))
            if (vm["depth-png"].as<std::string>() == "yes")
            {
                depthAsPng = true;
            }

        Oni2AviConverter converter;
        converter.run(vm["codec"].as<std::string>(),
                vm["input-file"].as<std::string>(),
                vm["output-file"].as<std::string>(), depthAsPng);
    }
    catch (const char* error)
    {
        // oni2avi errors
        std::cout << "Error: " << error << std::endl;
        return 1;
    }
    catch (const std::exception& error)
    {
        // OpenCV exceptions are derived from std::exception
        std::cout << error.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "Unknown error" << std::endl;
        return 1;
    }

    return 0;
}
