/* ImageColorAveraging.cpp
*
*  MIT License
*
*  Copyright (c) 2020-2026 awawa-dev
*
*  Project homesite: https://github.com/awawa-dev/HyperHDR
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#include <base/ImageColorAveraging.h>
#include <base/ImageToLedManager.h>
#include <infinite-color-engine/InfiniteProcessing.h>
#include <infinite-color-engine/ColorSpace.h>

#include <algorithm>
#include <array>
#include <limits>
#include <ranges>
#include <iterator>

#define push_back_index(list, index) list.push_back((index) * 3)

using namespace hyperhdr;
using namespace linalg::aliases;

ImageColorAveraging::ImageColorAveraging(
				const LoggerName& _log,
				const int mappingType,
				const bool sparseProcessing,
				const unsigned width,
				const unsigned height,
				const unsigned horizontalBorder,
				const unsigned verticalBorder,
				const quint8 /*instanceIndex*/,
				const std::vector<LedString::Led>& leds,
				const MappingParams& mappingParams)
	: _width(width)
	, _height(height)
	, _sparseProcessing(sparseProcessing)
	, _horizontalBorder(horizontalBorder)
	, _verticalBorder(verticalBorder)
	, _colorsMap()
	, _colorGroups()
	, _mappingParams(mappingParams)
{
	Q_ASSERT(_width > 2 * _verticalBorder);
	Q_ASSERT(_height > 2 * _horizontalBorder);
	Q_ASSERT(_width < 10000);
	Q_ASSERT(_height < 10000);

	_mappingType = mappingType;

	_colorsMap.reserve(leds.size());

	const int32_t xOffset = _verticalBorder;
	const int32_t actualWidth = _width - 2 * _verticalBorder;
	const int32_t yOffset = _horizontalBorder;
	const int32_t actualHeight = _height - 2 * _horizontalBorder;

	size_t   totalCount = 0;
	size_t   totalCapasity = 0;

	for (int ledIndex = -1; const LedString::Led& led : leds)
	{
		ledIndex++;

		if ((led.maxX_frac - led.minX_frac) < 1e-6 || (led.maxY_frac - led.minY_frac) < 1e-6)
		{
			_colorsMap.emplace_back();
			continue;
		}

		int32_t minX_idx = xOffset + int32_t(qRound(actualWidth * led.minX_frac));
		int32_t maxX_idx = xOffset + int32_t(qRound(actualWidth * led.maxX_frac));
		int32_t minY_idx = yOffset + int32_t(qRound(actualHeight * led.minY_frac));
		int32_t maxY_idx = yOffset + int32_t(qRound(actualHeight * led.maxY_frac));

		minX_idx = qMin(minX_idx, xOffset + actualWidth - 1);
		if (minX_idx == maxX_idx)
		{
			maxX_idx++;
		}
		minY_idx = qMin(minY_idx, yOffset + actualHeight - 1);
		if (minY_idx == maxY_idx)
		{
			maxY_idx++;
		}

		const int32_t maxYLedCount = qMin(maxY_idx, yOffset + actualHeight);
		const int32_t maxXLedCount = qMin(maxX_idx, xOffset + actualWidth);
		const int32_t realYLedCount = qAbs(maxYLedCount - minY_idx);
		const int32_t realXLedCount = qAbs(maxXLedCount - minX_idx);

		bool   sparseIndexes = sparseProcessing;
		size_t totalSize = static_cast<size_t>(realYLedCount) * realXLedCount;

		if (!sparseIndexes && totalSize > 1600)
		{
			Warning(_log, "This is large image area for lamp: {:d}. It contains {:d} indexes for captured video frame so reduce it by four. Enabling 'sparse processing' option for you. Consider to enable it permanently in the processing configuration to hide that warning.", ledIndex, totalSize);
			sparseIndexes = true;
		}

		const int32_t increment = (sparseIndexes) ? 2 : 1;

		totalCount += _colorsMap.emplace_back([&]{
			std::vector<uint32_t> ledColor;
			if (!led.disabled)
			{
				ledColor.reserve((sparseIndexes) ? ((static_cast<unsigned long long>(realYLedCount / 2)) + (realYLedCount % 2)) * ((realXLedCount / 2) + (realXLedCount % 2)) : totalSize);

				for (int32_t y = minY_idx; y < maxYLedCount; y += increment)
				{
					for (int32_t x = minX_idx; x < maxXLedCount; x += increment)
					{
						push_back_index(ledColor, y * width + x);
					}
				}
			}
			return ledColor;
		}()).size();
		totalCapasity += _colorsMap.back().capacity();

		if (led.group > 0)
		{
			if (_colorGroups.contains(led.group))
			{
				int master = _colorGroups[led.group].front();
				auto& dest_vec = _colorsMap[master];
				auto& source_vec = _colorsMap.back();

				dest_vec.insert(
					dest_vec.end(),
					std::make_move_iterator(source_vec.begin()),
					std::make_move_iterator(source_vec.end())
				);

				source_vec.clear();
			}
			_colorGroups[led.group].push_back(ledIndex);
		}
	}
	Info(_log, "Total index number is: {:d} (memory: {:d}). User sparse processing is: {:s}, image size: {:d} x {:d}, area number: {:d}",
		totalCount, totalCapasity, (sparseProcessing) ? "enabled" : "disabled", width, height, leds.size());
}

unsigned ImageColorAveraging::width() const
{
	return _width;
}

unsigned ImageColorAveraging::height() const
{
	return _height;
}

unsigned ImageColorAveraging::horizontalBorder() const
{
	return _horizontalBorder;
}

unsigned ImageColorAveraging::verticalBorder() const {
	return _verticalBorder;
}

void ImageColorAveraging::process(std::vector<float3>& ledColors, const Image<ColorRgb>& image)
{
	ledColors.clear();
	ledColors.reserve(_colorsMap.size());

	switch (_mappingType)
	{
		case 1: getUnicolorForLeds(ledColors, image);    break;
		case 2: getDominantForLeds(ledColors, image);    break;
		case 3: getOklabForLeds(ledColors, image);       break;
		case 4: getKMeansForLeds(ledColors, image);      break;
		case 5: getMultiSampleForLeds(ledColors, image); break;
		default: getMulticolorForLeds(ledColors, image);
	}

	if (!_colorGroups.empty() && _mappingType != 1)
	{
		for (auto& group : _colorGroups)
		{
			const auto& combined = ledColors[group.second.front()];
			for (int g = 1; g < static_cast<int>(group.second.size()); g++)
			{
				ledColors[group.second[g]] = combined;
			}
		}
	}
}

void ImageColorAveraging::getUnicolorForLeds(std::vector<float3>& ledColors, const Image<ColorRgb>& image) const
{
	ledColors.resize(_colorsMap.size(), calcUnicolorForLeds(image));
}

void ImageColorAveraging::getMulticolorForLeds(std::vector<float3>& ledColors, const Image<ColorRgb>& image) const
{
	for (auto colors = _colorsMap.begin(); colors != _colorsMap.end(); ++colors)
	{
		ledColors.push_back(calcMulticolorForLeds(image, *colors));
	}
}

float3 ImageColorAveraging::calcMulticolorForLeds(const Image<ColorRgb>& image, const std::vector<uint32_t>& colors) const
{
	if (colors.empty())
	{
		return float3{ 0, 0, 0 };
	}

	linalg::vec<uint_fast64_t, 3> sumLinear(0, 0, 0);

	const uint8_t* imgData = image.rawMem();

	for (const uint32_t colorOffset : colors)
	{
		sumLinear += InfiniteProcessing::srgbNonlinearToLinear(byte3(imgData[colorOffset], imgData[colorOffset + 1], imgData[colorOffset + 2]));
	}

	auto averageLinear = (static_cast<float3>(sumLinear) / static_cast<float>(colors.size())) / 65535.0f;

	return averageLinear;
}

float3 ImageColorAveraging::calcUnicolorForLeds(const Image<ColorRgb>& image) const
{
	uint_fast64_t sum = 0;
	linalg::vec<uint_fast64_t, 3> sumLinear(0, 0, 0);

	const uint8_t* imgData = image.rawMem();
	const uint32_t rowSize = image.width() * static_cast<uint32_t>(sizeof(ColorRgb));
	const uint32_t increment = (_sparseProcessing) ? 2 : 1;

	for (uint32_t y = 0; y < image.height(); y += increment)
	{
		for (uint32_t colorOffset = y * rowSize; colorOffset < y * rowSize + rowSize; colorOffset += increment * static_cast<uint32_t>(sizeof(ColorRgb)))
		{
			sumLinear += InfiniteProcessing::srgbNonlinearToLinear(byte3(imgData[colorOffset], imgData[colorOffset + 1], imgData[colorOffset + 2]));
			sum++;
		}
	}

	auto averageLinear = (static_cast<float3>(sumLinear) / static_cast<float>(sum)) / 65535.0f;

	return averageLinear;
}

// ===========================================================================
//  Dominant color extraction (Philips Hue / LG Ambilight style)
//
//  Instead of a plain average (which produces a muddy grey when a zone
//  contains multiple saturated colors + neutral areas), this extractor:
//    1. Quantizes pixels into HSV hue bins
//    2. Weights each pixel by saturation * value^gamma
//    3. Finds the dominant hue cluster (peak bin + neighbors)
//    4. Averages only pixels inside that cluster
// ===========================================================================

void ImageColorAveraging::getDominantForLeds(std::vector<float3>& ledColors, const Image<ColorRgb>& image) const
{
	ledColors.reserve(_colorsMap.size());
	for (const auto& colors : _colorsMap)
	{
		ledColors.push_back(calcDominantForLeds(image, colors));
	}
}

float3 ImageColorAveraging::calcDominantForLeds(const Image<ColorRgb>& image, const std::vector<uint32_t>& colors) const
{
	if (colors.empty())
	{
		return float3{ 0, 0, 0 };
	}

	// Pull the tunable parameters (see ImageColorAveraging.h)
	const int   HUE_BINS          = std::max(4, std::min(_mappingParams.hueBins, 64));
	const int   NEIGHBOR_RADIUS   = std::max(0, std::min(_mappingParams.neighborRadius, 4));
	const float MIN_SATURATION    = std::clamp(_mappingParams.minSaturation, 0.0f, 1.0f);
	const float VALUE_GAMMA       = std::clamp(_mappingParams.valueGamma, 0.05f, 2.0f);
	const float NEUTRAL_WEIGHT    = std::clamp(_mappingParams.neutralWeight, 0.0f, 1.0f);
	const float SATURATION_WEIGHT = std::clamp(_mappingParams.saturationWeight, 0.0f, 10.0f);

	const float binWidth = 360.0f / static_cast<float>(HUE_BINS);

	// Per-bin accumulators. Weights are stored as fixed-point (x1000)
	// to stay in integer arithmetic for the RGB sum, matching the
	// existing multicolor code path.
	std::vector<uint_fast64_t>                 binWeight(HUE_BINS, 0);
	std::vector<linalg::vec<uint_fast64_t, 3>> binSum(HUE_BINS, linalg::vec<uint_fast64_t, 3>{ 0, 0, 0 });

	uint_fast64_t                 neutralWeight = 0;
	linalg::vec<uint_fast64_t, 3> neutralSum{ 0, 0, 0 };

	const uint8_t* imgData = image.rawMem();

	for (const uint32_t colorOffset : colors)
	{
		const uint8_t r8 = imgData[colorOffset];
		const uint8_t g8 = imgData[colorOffset + 1];
		const uint8_t b8 = imgData[colorOffset + 2];

		const auto linear = InfiniteProcessing::srgbNonlinearToLinear(byte3(r8, g8, b8));

		// sRGB -> HSV for hue analysis
		const float3 rgbNl = { r8 / 255.0f, g8 / 255.0f, b8 / 255.0f };
		const float3 hsv   = ColorSpaceMath::rgb2hsv(rgbNl);

		const float h = hsv.x;   // 0..360
		const float s = hsv.y;   // 0..1
		const float v = hsv.z;   // 0..1

		// Weight: saturated & bright pixels dominate
		const float satFactor = 1.0f + s * SATURATION_WEIGHT;
		const float valFactor = std::pow(std::max(v, 0.02f), VALUE_GAMMA);
		const float weight    = satFactor * valFactor;

		const uint_fast64_t weightQ = static_cast<uint_fast64_t>(weight * 1000.0f + 0.5f);

		if (s < MIN_SATURATION)
		{
			neutralWeight += weightQ;
			neutralSum    += linear * weightQ;
			continue;
		}

		int bin = static_cast<int>(h / binWidth);
		if (bin < 0) bin = 0;
		if (bin >= HUE_BINS) bin = HUE_BINS - 1;

		binWeight[bin] += weightQ;
		binSum[bin]    += linear * weightQ;
	}

	// Find the peak bin
	int            peakBin    = 0;
	uint_fast64_t  peakWeight = 0;
	for (int i = 0; i < HUE_BINS; ++i)
	{
		if (binWeight[i] > peakWeight)
		{
			peakWeight = binWeight[i];
			peakBin    = i;
		}
	}

	// If everything was neutral (grey scene), fall back to plain average
	if (peakWeight == 0)
	{
		return calcMulticolorForLeds(image, colors);
	}

	// Sum the dominant cluster (peak bin +- neighbors)
	uint_fast64_t                 clusterWeight = 0;
	linalg::vec<uint_fast64_t, 3> clusterSum{ 0, 0, 0 };
	for (int d = -NEIGHBOR_RADIUS; d <= NEIGHBOR_RADIUS; ++d)
	{
		const int idx = (peakBin + d + HUE_BINS) % HUE_BINS;
		clusterWeight += binWeight[idx];
		clusterSum    += binSum[idx];
	}

	// Add a small neutral contribution so fully desaturated zones aren't crushed
	if (neutralWeight > 0 && NEUTRAL_WEIGHT > 0.0f)
	{
		const uint_fast64_t nwQ = static_cast<uint_fast64_t>(NEUTRAL_WEIGHT * 1000.0f + 0.5f);
		clusterWeight += (neutralWeight * nwQ) / 1000ULL;
		clusterSum    += (neutralSum * nwQ) / 1000ULL;
	}

	if (clusterWeight == 0)
	{
		return calcMulticolorForLeds(image, colors);
	}

	// Weighted average in linear space, then normalized to 0..1
	const float3 averageLinear =
		(static_cast<float3>(clusterSum) / static_cast<float>(clusterWeight)) / 65535.0f;

	return linalg::clamp(averageLinear, 0.0f, 1.0f);
}

// ===========================================================================
//  Oklab average (perceptually uniform averaging)
//
//  Averages the zone pixels in the Oklab color space instead of RGB.
//  The result is perceptually correct and avoids muddy mid-tones when
//  a zone mixes two saturated colors.
// ===========================================================================

void ImageColorAveraging::getOklabForLeds(std::vector<float3>& ledColors, const Image<ColorRgb>& image) const
{
	ledColors.reserve(_colorsMap.size());
	for (const auto& colors : _colorsMap)
	{
		ledColors.push_back(calcOklabForLeds(image, colors));
	}
}

float3 ImageColorAveraging::calcOklabForLeds(const Image<ColorRgb>& image, const std::vector<uint32_t>& colors) const
{
	if (colors.empty())
	{
		return float3{ 0, 0, 0 };
	}

	// Average in Oklab: perceptually uniform, no muddy mid-tones.
	float3 sumOklab{ 0, 0, 0 };
	const uint8_t* imgData = image.rawMem();

	for (const uint32_t colorOffset : colors)
	{
		const auto linear = InfiniteProcessing::srgbNonlinearToLinear(
			byte3(imgData[colorOffset], imgData[colorOffset + 1], imgData[colorOffset + 2]));

		const float3 lin = static_cast<float3>(linear) / 65535.0f;
		sumOklab += ColorSpaceMath::linear_rgb_to_oklab(lin);
	}

	const float inv = 1.0f / static_cast<float>(colors.size());
	const float3 avgOklab  = sumOklab * inv;
	const float3 avgLinear = ColorSpaceMath::oklab_to_linear_rgb(avgOklab);

	return linalg::clamp(avgLinear, 0.0f, 1.0f);
}

// ===========================================================================
//  K-means (k=2)
//
//  Clusters the zone pixels into 2 groups using Lloyd's algorithm and
//  returns the average of the bigger cluster. Useful when the zone mixes
//  two roughly equally important colors and plain averaging would wash
//  them out.
// ===========================================================================

void ImageColorAveraging::getKMeansForLeds(std::vector<float3>& ledColors, const Image<ColorRgb>& image) const
{
	ledColors.reserve(_colorsMap.size());
	for (const auto& colors : _colorsMap)
	{
		ledColors.push_back(calcKMeansForLeds(image, colors));
	}
}

float3 ImageColorAveraging::calcKMeansForLeds(const Image<ColorRgb>& image, const std::vector<uint32_t>& colors) const
{
	if (colors.empty())
	{
		return float3{ 0, 0, 0 };
	}

	const size_t n = colors.size();
	if (n < 4)
	{
		return calcMulticolorForLeds(image, colors);
	}

	const uint8_t* imgData = image.rawMem();

	// Helper: 16-bit linear -> float [0..1]
	auto toFloat = [](const linalg::vec<uint16_t, 3>& v) -> float3 {
		return float3{
			static_cast<float>(v.x) / 65535.0f,
			static_cast<float>(v.y) / 65535.0f,
			static_cast<float>(v.z) / 65535.0f
		};
	};

	// Seed the two centroids using two well-separated pixels
	float3 c0{ 0, 0, 0 };
	float3 c1{ 0, 0, 0 };
	{
		const auto& first = colors[0];
		c0 = toFloat(InfiniteProcessing::srgbNonlinearToLinear(
			byte3(imgData[first], imgData[first + 1], imgData[first + 2])));

		// Find the pixel furthest from c0
		float bestDist = -1.0f;
		for (size_t i = 1; i < n; ++i)
		{
			const float3 lin = toFloat(InfiniteProcessing::srgbNonlinearToLinear(
				byte3(imgData[colors[i]], imgData[colors[i] + 1], imgData[colors[i] + 2])));
			const float3 d = lin - c0;
			const float dist = linalg::dot(d, d);
			if (dist > bestDist) { bestDist = dist; c1 = lin; }
		}
	}

	size_t n0 = 0;
	size_t n1 = 0;

	// Lloyd's algorithm, 4 iterations
	for (int iter = 0; iter < 4; ++iter)
	{
		float3 s0{ 0, 0, 0 };
		float3 s1{ 0, 0, 0 };
		n0 = 0;
		n1 = 0;

		for (const uint32_t off : colors)
		{
			const float3 lin = toFloat(InfiniteProcessing::srgbNonlinearToLinear(
				byte3(imgData[off], imgData[off + 1], imgData[off + 2])));

			const float3 d0 = lin - c0;
			const float3 d1 = lin - c1;
			const float dist0 = linalg::dot(d0, d0);
			const float dist1 = linalg::dot(d1, d1);

			if (dist0 < dist1) { s0 = s0 + lin; n0++; }
			else               { s1 = s1 + lin; n1++; }
		}

		if (n0 > 0) c0 = s0 / static_cast<float>(n0);
		if (n1 > 0) c1 = s1 / static_cast<float>(n1);
	}

	// Return the bigger cluster (dominant population)
	const float3 bigger = (n0 >= n1) ? c0 : c1;
	return linalg::clamp(bigger, 0.0f, 1.0f);
}

// ===========================================================================
//  Multi-sample voting
//
//  Splits the zone pixels into 4 sub-samples and computes a dominant
//  color for each. The sample that is closest to all others wins.
//  Useful for very large zones (65" TV corners) where 4 sub-regions may
//  contain very different content.
// ===========================================================================

void ImageColorAveraging::getMultiSampleForLeds(std::vector<float3>& ledColors, const Image<ColorRgb>& image) const
{
	ledColors.reserve(_colorsMap.size());
	for (const auto& colors : _colorsMap)
	{
		ledColors.push_back(calcMultiSampleForLeds(image, colors));
	}
}

float3 ImageColorAveraging::calcMultiSampleForLeds(const Image<ColorRgb>& image, const std::vector<uint32_t>& colors) const
{
	if (colors.empty())
	{
		return float3{ 0, 0, 0 };
	}

	const size_t n = colors.size();
	if (n < 16)
	{
		return calcMulticolorForLeds(image, colors);
	}

	// Split into 4 chunks (index-based, works regardless of pixel ordering)
	const size_t quarter = n / 4;

	float3 samples[4];
	for (int q = 0; q < 4; ++q)
	{
		std::vector<uint32_t> sub(colors.begin() + q * quarter,
		                          colors.begin() + (q + 1) * quarter);
		samples[q] = calcDominantForLeds(image, sub);
	}

	// Vote: the sample closest to all others wins
	int best = 0;
	float bestScore = std::numeric_limits<float>::max();
	for (int i = 0; i < 4; ++i)
	{
		float score = 0.0f;
		for (int j = 0; j < 4; ++j)
		{
			if (i == j) continue;
			const float3 d = samples[i] - samples[j];
			score += linalg::dot(d, d);
		}
		if (score < bestScore) { bestScore = score; best = i; }
	}

	return samples[best];
}