// (C) Copyright Kirill Lykov 2013.
//
// Distributed under the FreeBSD Software License (See accompanying file license.txt)

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

// OpenNI
#include <XnCppWrapper.h>
#define THROW_IF_FAILED(retVal) {if (retVal != XN_STATUS_OK) throw xnGetStatusString(retVal);}

// OpenCV
#include <opencv2/opencv.hpp>

class CodecName2FourCC
{
  typedef std::map<std::string, std::string> Map;
  typedef typename std::map<std::string, std::string>::iterator Iterator;
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
    xn::Context context;
    THROW_IF_FAILED(context.Init());

    xn::Player player;
    THROW_IF_FAILED(context.OpenFileRecording(inputFile.c_str(), player));
    THROW_IF_FAILED(player.SetRepeat(false));

    xn::ImageGenerator imageGen;
    THROW_IF_FAILED(imageGen.Create(context));
    XnPixelFormat pixelFormat = imageGen.GetPixelFormat();
    if (pixelFormat != XN_PIXEL_FORMAT_RGB24) {
      THROW_IF_FAILED(imageGen.SetPixelFormat(XN_PIXEL_FORMAT_RGB24));
    }
    xn::ImageMetaData xImageMap2;
    imageGen.GetMetaData(xImageMap2);
    XnUInt32 fps = xImageMap2.FPS();
    XnUInt32 frame_height = xImageMap2.YRes();
    XnUInt32 frame_width = xImageMap2.XRes();

    xn::DepthGenerator depthGen;
    depthGen.Create(context);
    XnUInt32 nframes;
    player.GetNumFrames(depthGen.GetName(), nframes);

    printResume(nframes, codecName, inputFile, outputFile);

    std::string outputFileImg, outputFileDepth;
    getOutputFileNames(outputFile, outputFileImg, outputFileDepth);

    cv::VideoWriter imgWriter(outputFileImg, m_codecName2Code(codecName), fps, cvSize(frame_width, frame_height), 1);
    cv::VideoWriter depthWriter(outputFileDepth, m_codecName2Code(codecName), fps, cvSize(frame_width, frame_height), 1);

    std::string depthFolderName = getDepthFolderName(outputFileImg);
    fs::path folderForDepthImages(depthFolderName);
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

    THROW_IF_FAILED(context.StartGeneratingAll());

    size_t outStep = nframes / 10;

    try
    {
      for(size_t iframe = 0; iframe < nframes; ++iframe)
      {
        if ( iframe % outStep == 0 )
            std::cout << iframe << "/" << nframes << std::endl;

        // save image
        THROW_IF_FAILED(imageGen.WaitAndUpdateData());
        xn::ImageMetaData xImageMap;
        imageGen.GetMetaData(xImageMap);
        XnRGB24Pixel* imgData = const_cast<XnRGB24Pixel*>(xImageMap.RGB24Data());
        cv::Mat image(frame_height, frame_width, CV_8UC3, reinterpret_cast<void*>(imgData));

        cv::cvtColor(image, image, CV_BGR2RGB); // opencv image format is bgr
        imgWriter << image.clone();

        // save depth
        THROW_IF_FAILED(depthGen.WaitAndUpdateData());
        xn::DepthMetaData xDepthMap;
        depthGen.GetMetaData(xDepthMap);
        XnDepthPixel* depthData = const_cast<XnDepthPixel*>(xDepthMap.Data());
        cv::Mat depth(frame_height, frame_width, CV_16U, reinterpret_cast<void*>(depthData));

        if (!depthAsPng)
        {
  #if (CV_MAJOR_VERSION == 2 && CV_MINOR_VERSION >= 4) || CV_MAJOR_VERSION > 2
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
          ss << depthFolderName << "/depth-" << iframe << ".png";
          cv::imwrite(ss.str(), depth);
        }
      }
    }
    catch(...)
    {
      context.StopGeneratingAll();
      context.Release();
      throw;
    }

    context.StopGeneratingAll();
    context.Release();
  }

private:

  static void printResume(size_t nframes, const std::string& codecName,
      const std::string& inputFile, const std::string& outputFile)
  {
    std::cout << "Total: " << nframes << " frames. Used codec: " << codecName  <<
        ".\n Input file name: " << inputFile << ". Output file name: " << outputFile << "-img/depth" << std::endl;
  }

  static std::string getFileNameNoExt(const std::string& outputFileName)
  {
    size_t index = outputFileName.find_last_of('.');
    std::string nameWithoutExtension;
    if (index == std::string::npos)
      nameWithoutExtension = outputFileName;
    nameWithoutExtension = outputFileName.substr(0, index);
    std::string extention = outputFileName.substr(index + 1);
    if (extention != "avi")
      throw "output file extention must be avi";
    return nameWithoutExtension;
  }

  static void getOutputFileNames(const std::string& outputFileName, std::string& outputFileImg,
                                 std::string& outputFileDepth)
  {
    std::string nameWithoutExtension = getFileNameNoExt(outputFileName);

    outputFileImg = nameWithoutExtension + "-img.avi";
    outputFileDepth = nameWithoutExtension + "-depth.avi";
  }

  static std::string getDepthFolderName(const std::string& outputFileName)
  {
    std::string nameWithoutExtension = getFileNameNoExt(outputFileName);
    return nameWithoutExtension + "-depth";
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
