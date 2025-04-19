#include <opencv2/opencv.hpp>
#include <filesystem>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <conio.h>

namespace fs = std::filesystem;

const int TILE_SIZE = 256;
std::string endFileType = "webp";

static void saveTile(const cv::Mat& tile, int zoom, int x, int y, const std::string& outputDir) {
	fs::path tilePath = fs::path(outputDir) / std::to_string(zoom) / std::to_string(x);
	fs::create_directories(tilePath);
	std::string filename = (tilePath / (std::to_string(y) + "." + endFileType)).string();
	cv::imwrite(filename, tile);
}

static void processColumn(const cv::Mat& scaledImage, int zoom, int x, int maxY, const std::string& outputDir) {
	for (int y = 0; y < maxY; ++y) {
		int tileX = x * TILE_SIZE;
		int tileY = y * TILE_SIZE;
		cv::Rect roi(tileX, tileY, TILE_SIZE, TILE_SIZE);

		cv::Mat tile;
		if (tileX + TILE_SIZE <= scaledImage.cols && tileY + TILE_SIZE <= scaledImage.rows)
			tile = scaledImage(roi);
		else {
			tile = cv::Mat::zeros(TILE_SIZE, TILE_SIZE, scaledImage.type());
			cv::Mat sub = scaledImage(cv::Rect(tileX, tileY,
				std::min(TILE_SIZE, scaledImage.cols - tileX),
				std::min(TILE_SIZE, scaledImage.rows - tileY)));
			sub.copyTo(tile(cv::Rect(0, 0, sub.cols, sub.rows)));
		}

		saveTile(tile, zoom, x, y, outputDir);
	}
}

static void processZoomLevel(const cv::Mat& image, int zoom, const std::string& outputDir) {
	int scale = 1 << zoom;
	int size = TILE_SIZE * scale;
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(size, size), 0, 0, cv::INTER_AREA);

	int maxX = (resized.cols + TILE_SIZE - 1) / TILE_SIZE;
	int maxY = (resized.rows + TILE_SIZE - 1) / TILE_SIZE;

	if (zoom >= 5) {
		std::vector<std::thread> threads;
		for (int x = 0; x < maxX; ++x) {
			threads.emplace_back(processColumn, std::cref(resized), zoom, x, maxY, std::cref(outputDir));
		}
		for (auto& t : threads) t.join();
	}
	else {
		for (int x = 0; x < maxX; ++x) {
			processColumn(resized, zoom, x, maxY, outputDir);
		}
	}
}

int main() {
	std::cout << "Image: ";
	std::string imagePath;
	std::cin >> imagePath;

	std::cout << "Attempting to load image: '" << imagePath << "'... (Large images could take a moment)\n";
	cv::Mat image = cv::imread(imagePath);
	if (image.empty()) {
		std::cerr << "Error: Could not load image.\n";
		return 1;
	}

	std::cout << "Zoom Level: ";
	std::string zoomString;
	std::cin >> zoomString;
	int maxZoom = std::stoi(zoomString);

    std::cout << "End File Type: ";
	std::string newEndFile;
	std::cin >> newEndFile;

    if (newEndFile.size() == 0) endFileType = newEndFile;

	std::string outputDir = "tiles";

	for (int i = 0; i <= maxZoom; i++) {
		std::cout << "Processing zoom level " << i << "...\n";
		processZoomLevel(image, i, outputDir);
	}

	std::cout << "Tiling complete." << std::endl;
    std::cout << "Press any key to exit..." << std::endl;
    
    _getch(); // Wait for user input before exiting
	return 0;
}