#include <iostream>
#include <vector>
#include <algorithm>
#include "SignalProcessor.h"
#include "FFTProcessor.h"
#include "../include/kissfft/kiss_fft.h"
#include "../include/kissfft/kiss_fftr.h"

// Helper method to filter frequencies within a given range
std::vector<kiss_fft_cpx> SignalProcessor::filterFrequencyComponents(
    const std::vector<kiss_fft_cpx>& fftOutput,
    const std::vector<double>& frequencies,
    double lowFreq, double highFreq) {

    std::vector<kiss_fft_cpx> filteredOutput(fftOutput.size());
    for (size_t i = 0; i < frequencies.size(); ++i) {
        if (frequencies[i] >= lowFreq && frequencies[i] <= highFreq) {
            filteredOutput[i] = fftOutput[i];
        } else {
            filteredOutput[i].r = 0.0;
            filteredOutput[i].i = 0.0;
        }
    }
    return filteredOutput;
}

// Helper function to compare two signals
// Not sure if it's working or not
bool SignalProcessor::compareSignals(
    const std::vector<double>& original, 
    const std::vector<double>& processed, 
    double tolerance) {
    if (original.size() != processed.size()) {
        std::cerr << "Signal sizes do not match!" << std::endl;
        return false;
    }

    for (size_t i = 0; i < original.size(); ++i) {
        if (std::abs(original[i] - processed[i]) > tolerance) {
            std::cerr << "Signals differ at index " << i << ": "
                      << original[i] << " vs " << processed[i] << std::endl;
            return false;
        }
    }

    return true;
}

std::vector<double> SignalProcessor::calculateFrequencies(
    size_t fftSize,
    double sampleRate,
    const std::vector<kiss_fft_cpx>& fftOutput) {
    std::vector<double> frequencies;
    for (size_t i = 0; i < fftSize / 2; ++i) { // Only up to Nyquist frequency
        double frequency = static_cast<double>(i) * sampleRate / fftSize; //
        frequencies.push_back(frequency);
    }
    return frequencies;
}

std::vector<double> SignalProcessor::calculateMagnitudes(
    size_t fftSize,
    double sampleRate,
    const std::vector<kiss_fft_cpx>& fftOutput) {
    std::vector<double> magnitudes;
    for (size_t i = 0; i < fftSize / 2; ++i) { // Only up to Nyquist frequency
        double magnitude = std::sqrt(fftOutput[i].r * fftOutput[i].r + fftOutput[i].i * fftOutput[i].i);
        magnitudes.push_back(magnitude);
    }
    return magnitudes;
}

// method for chunking, processing and reconstructing audio
// given a signal, in chunks of fftsize, performs fft, gets frequencies,
// filters frequencies within a range, performs ifft, then reconstructs into filtered signal
std::vector<double> SignalProcessor::extractAudioFromChunks(
    const std::vector<double>& originalSignal, 
    size_t fftSize, 
    double sampleRate,
    double lowFreq, 
    double highFreq,
    FFTProcessor& fftProcessor  // Pass FFTProcessor as a reference
) {
    size_t signalSize = originalSignal.size();
    std::vector<double> extractedSignal;
    size_t numChunks = signalSize / fftSize;

    // Process each chunk of fftSize
    for (size_t i = 0; i < numChunks; ++i) {
        std::vector<double> chunk(originalSignal.begin() + i * fftSize, 
                                   originalSignal.begin() + (i + 1) * fftSize);
        fftProcessor.performFFT(chunk);
        const std::vector<kiss_fft_cpx>& fftOutput = fftProcessor.getFFTOutput();
        std::vector<double> frequencies = calculateFrequencies(fftSize, sampleRate, fftOutput);
        std::vector<kiss_fft_cpx> filteredOutput = filterFrequencyComponents(fftOutput, frequencies, lowFreq, highFreq);
        std::vector<double> ifftOutput = fftProcessor.performIFFT(filteredOutput);  // Call the existing IFFT method
        extractedSignal.insert(extractedSignal.end(), ifftOutput.begin(), ifftOutput.end());
    }

    return extractedSignal;
}