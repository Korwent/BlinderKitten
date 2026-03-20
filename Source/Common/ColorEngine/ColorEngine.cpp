/*
  ==============================================================================

    ColorEngine.cpp
    Created: 10 Mar 2026
    Author:  BlinderKitten

  ==============================================================================
*/

#include "ColorEngine.h"
#include "../../Definitions/SubFixture/SubFixture.h"
#include "../../Definitions/SubFixture/SubFixtureChannel.h"
#include "../../Definitions/ChannelFamily/ChannelType/ChannelType.h"
#include "../../Definitions/FixtureType/FixtureTypeChannel.h"
#include "../../Definitions/Fixture/Fixture.h"

// ===== CIE Conversion Functions =====

void ColorEngine::sRGBtoXYZ(float r, float g, float b, float& X, float& Y, float& Z)
{
    // Linearize sRGB (IEC 61966-2-1)
    auto linearize = [](float c) -> float {
        if (c <= 0.04045f) return c / 12.92f;
        return powf((c + 0.055f) / 1.055f, 2.4f);
    };

    float lr = linearize(r);
    float lg = linearize(g);
    float lb = linearize(b);

    // Linear RGB → CIE XYZ (D65, 2° observer)
    X = 0.4124564f * lr + 0.3575761f * lg + 0.1804375f * lb;
    Y = 0.2126729f * lr + 0.7151522f * lg + 0.0721750f * lb;
    Z = 0.0193339f * lr + 0.1191920f * lg + 0.9503041f * lb;
}

void ColorEngine::XYZtoSRGB(float X, float Y, float Z, float& r, float& g, float& b)
{
    // CIE XYZ → linear RGB (inverse matrix for D65)
    float lr =  3.2404542f * X - 1.5371385f * Y - 0.4985314f * Z;
    float lg = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
    float lb =  0.0556434f * X - 0.2040259f * Y + 1.0572252f * Z;

    // Apply gamma (sRGB)
    auto gammify = [](float c) -> float {
        if (c <= 0.0031308f) return 12.92f * c;
        return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
    };

    r = gammify(lr);
    g = gammify(lg);
    b = gammify(lb);

    // Clamp to [0,1]
    r = jlimit(0.0f, 1.0f, r);
    g = jlimit(0.0f, 1.0f, g);
    b = jlimit(0.0f, 1.0f, b);
}

void ColorEngine::xyYtoXYZ(float x, float y, float Yval, float& X, float& Y, float& Z)
{
    if (y < 0.0001f) {  // Avoid division by zero
        X = Y = Z = 0.0f;
        return;
    }
    X = (x * Yval) / y;
    Y = Yval;
    Z = ((1.0f - x - y) * Yval) / y;
}

bool ColorEngine::XYZtoXY(float X, float Y, float Z, float& x, float& y)
{
    float sum = X + Y + Z;
    if (sum < 0.0001f) return false;  // Avoid division by zero

    x = X / sum;
    y = Y / sum;
    return true;
}

// ===== Calibration Collection =====

Array<EmitterCalibration> ColorEngine::getCalibrations(SubFixture* sf)
{
    Array<EmitterCalibration> result;

    if (sf == nullptr) return result;

    for (auto it = sf->channelsMap.begin(); it != sf->channelsMap.end(); it.next()) {
        SubFixtureChannel* sfc = it.getValue();
        if (sfc == nullptr || sfc->parentFixtureTypeChannel == nullptr) continue;

        FixtureTypeChannel* ftc = sfc->parentFixtureTypeChannel;

        // Check if calibration CIE xy is enabled
        if (ftc->calibrationCIExy == nullptr || !ftc->calibrationCIExy->enabled) continue;

        // Check if max intensity is enabled and > 0
        if (ftc->calibrationMaxIntensity == nullptr || !ftc->calibrationMaxIntensity->enabled) continue;
        if (ftc->calibrationMaxIntensity->floatValue() <= 0.0f) continue;

        // Build EmitterCalibration struct
        EmitterCalibration ec;
        ec.channel = sfc;
        ec.typeChannel = ftc;
        ec.cieX = ftc->calibrationCIExy->x;
        ec.cieY = ftc->calibrationCIExy->y;
        ec.maxIntensityCd = ftc->calibrationMaxIntensity->floatValue();

        // Compute full-intensity tristimulus XYZ
        xyYtoXYZ(ec.cieX, ec.cieY, ec.maxIntensityCd, ec.fullX, ec.fullY, ec.fullZ);

        result.add(ec);
    }

    return result;
}

bool ColorEngine::hasCalibrationData(SubFixture* sf)
{
    return getCalibrations(sf).size() >= 3;
}

// ===== 3x3 Direct Solver (Cramer's Rule) =====

bool ColorEngine::solve3x3(const Array<EmitterCalibration>& emitters,
                            float targetX, float targetY, float targetZ,
                            Array<float>& outWeights)
{
    // Build matrix M where each column is [X_i, Y_i, Z_i]
    // M = [X0 X1 X2]   target = [targetX]
    //     [Y0 Y1 Y2]              [targetY]
    //     [Z0 Z1 Z2]              [targetZ]

    float m00 = emitters[0].fullX, m01 = emitters[1].fullX, m02 = emitters[2].fullX;
    float m10 = emitters[0].fullY, m11 = emitters[1].fullY, m12 = emitters[2].fullY;
    float m20 = emitters[0].fullZ, m21 = emitters[1].fullZ, m22 = emitters[2].fullZ;

    // Compute determinant
    float det = m00 * (m11 * m22 - m12 * m21)
              - m01 * (m10 * m22 - m12 * m20)
              + m02 * (m10 * m21 - m11 * m20);

    if (fabsf(det) < 0.00001f) return false;  // Singular matrix (coplanar emitters)

    // Cramer's rule: solve for w0, w1, w2
    float det0 = targetX * (m11 * m22 - m12 * m21)
               - m01 * (targetY * m22 - m12 * targetZ)
               + m02 * (targetY * m21 - m11 * targetZ);

    float det1 = m00 * (targetY * m22 - m12 * targetZ)
               - targetX * (m10 * m22 - m12 * m20)
               + m02 * (m10 * targetZ - targetY * m20);

    float det2 = m00 * (m11 * targetZ - targetY * m21)
               - m01 * (m10 * targetZ - targetY * m20)
               + targetX * (m10 * m21 - m11 * m20);

    float w0 = det0 / det;
    float w1 = det1 / det;
    float w2 = det2 / det;

    // Check if weights are in [0, 1] (in gamut)
    if (w0 < -0.001f || w0 > 1.001f ||
        w1 < -0.001f || w1 > 1.001f ||
        w2 < -0.001f || w2 > 1.001f) {
        // Out of gamut — clamp and return false
        w0 = jlimit(0.0f, 1.0f, w0);
        w1 = jlimit(0.0f, 1.0f, w1);
        w2 = jlimit(0.0f, 1.0f, w2);
    }

    outWeights.add(w0);
    outWeights.add(w1);
    outWeights.add(w2);

    return true;
}

// ===== Multi-Emitter Solver =====

bool ColorEngine::solveEmitterWeights(Array<EmitterCalibration>& emitters,
                                       float targetX, float targetY, float targetZ,
                                       Array<float>& outWeights)
{
    outWeights.clear();
    int n = emitters.size();

    if (n < 3) {
        // Underdetermined system
        for (int i = 0; i < n; i++) outWeights.add(0.0f);
        return false;
    }

    if (n == 3) {
        // Exact solve via Cramer's rule
        return solve3x3(emitters, targetX, targetY, targetZ, outWeights);
    }

    // n > 3: Try all C(n,3) combinations, pick the one with minimum total power in gamut
    float bestTotalPower = 1e9f;
    Array<float> bestWeights;
    int bestI = 0, bestJ = 1, bestK = 2;
    bool foundValid = false;

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            for (int k = j + 1; k < n; k++) {
                // Try combination (i, j, k)
                Array<EmitterCalibration> subset;
                subset.add(emitters[i]);
                subset.add(emitters[j]);
                subset.add(emitters[k]);

                Array<float> weights;
                if (solve3x3(subset, targetX, targetY, targetZ, weights)) {
                    // Check if all weights in [0, 1]
                    bool inGamut = true;
                    for (float w : weights) {
                        if (w < -0.001f || w > 1.001f) {
                            inGamut = false;
                            break;
                        }
                    }

                    if (inGamut) {
                        float totalPower = weights[0] + weights[1] + weights[2];
                        if (totalPower < bestTotalPower) {
                            bestTotalPower = totalPower;
                            bestWeights = weights;
                            bestI = i; bestJ = j; bestK = k;
                            foundValid = true;
                        }
                    }
                }
            }
        }
    }

    if (foundValid) {
        for (int i = 0; i < n; i++) outWeights.add(0.0f);
        outWeights.set(bestI, bestWeights[0]);
        outWeights.set(bestJ, bestWeights[1]);
        outWeights.set(bestK, bestWeights[2]);
        return true;
    }

    // No valid solution found — out of gamut
    for (int i = 0; i < n; i++) outWeights.add(0.0f);
    return false;
}

// ===== Dimming Curve Inverse =====

float ColorEngine::weightToDMXLevel(const EmitterCalibration& emitter, float weight)
{
    weight = jlimit(0.0f, 1.0f, weight);

    // No calibration dimming curve implemented yet — use linear mapping.
    // NOTE: Do NOT use emitter.typeChannel->curve here; that is the channel
    // output automation which is already applied in the DMX write pipeline
    // (SubFixtureChannel::writeValue). Using it here would double-apply it.
    return weight;
}

// ===== High-Level API =====

static void normalizeWeights(Array<EmitterCalibration>& emitters, Array<float>& weights,
                             float intensity, bool constantLuminance)
{
    if (constantLuminance) {
        // Compute the total luminance (Y) produced by the current weight vector.
        // Then scale so that Y_produced == intensity * Y_max_single_emitter_avg,
        // keeping chromaticity ratios intact.
        float solvedY = 0.0f;
        float maxY = 0.0f;
        for (int i = 0; i < emitters.size() && i < weights.size(); i++) {
            solvedY += emitters[i].fullY * weights[i];
            maxY += emitters[i].fullY;
        }
        if (solvedY > 0.0f && maxY > 0.0f) {
            float scale = intensity / solvedY;
            // Clamp scale so no weight exceeds 1.0
            float maxW = 0.0f;
            for (int i = 0; i < weights.size(); i++)
                maxW = jmax(maxW, weights[i] * scale);
            if (maxW > 1.0f) scale /= maxW;
            for (int i = 0; i < weights.size(); i++)
                weights.set(i, weights[i] * scale);
        }
    } else {
        // Max-weight normalization: dominant emitter = intensity
        float maxW = 0.0f;
        for (float w : weights) maxW = jmax(maxW, w);
        if (maxW > 0.0f) {
            for (int i = 0; i < weights.size(); i++)
                weights.set(i, (weights[i] / maxW) * intensity);
        }
    }
}

bool ColorEngine::applyTargetColorToSubFixture(SubFixture* sf,
                                                float r, float g, float b,
                                                float intensity,
                                                bool constantLuminance)
{
    Array<EmitterCalibration> emitters = getCalibrations(sf);
    if (emitters.size() < 3) return false;

    // Convert target sRGB to CIE XYZ (normalized, Y in 0..1 range)
    float X, Y, Z;
    sRGBtoXYZ(r, g, b, X, Y, Z);

    // Solve for emitter weights that reproduce this chromaticity.
    Array<float> weights;
    solveEmitterWeights(emitters, X, Y, Z, weights);

    // Normalize weights according to the caller's chosen mode
    normalizeWeights(emitters, weights, intensity, constantLuminance);

    // Convert weights to DMX levels and write to channels
    for (int i = 0; i < emitters.size() && i < weights.size(); i++) {
        float dmxLevel = weightToDMXLevel(emitters[i], weights[i]);
        // NOTE: In the actual system, these values would be set via Commands/Programmers
        // This is a placeholder; the integration with Command system will handle the actual setting
    }

    return true;
}

Colour ColorEngine::getCalibrationOutputColor(SubFixture* sf)
{
    Array<EmitterCalibration> emitters = getCalibrations(sf);
    if (emitters.size() < 3) return Colour(0, 0, 0);  // Uncalibrated

    float totalX = 0.0f, totalY = 0.0f, totalZ = 0.0f;

    for (const auto& e : emitters) {
        if (e.channel == nullptr) continue;

        // Use currentValue directly as relative intensity (linear).
        // Do NOT apply typeChannel->curve here — it is the output automation
        // already applied in the DMX write pipeline.
        float relativeIntensity = e.channel->currentValue;

        totalX += e.fullX * relativeIntensity;
        totalY += e.fullY * relativeIntensity;
        totalZ += e.fullZ * relativeIntensity;
    }

    // Normalize luminance
    float maxY = 0.0f;
    for (const auto& e : emitters) {
        maxY += e.fullY;
    }

    if (maxY > 0.0f) {
        totalX /= maxY;
        totalY /= maxY;
        totalZ /= maxY;
    }

    // Convert XYZ to sRGB
    float r, g, b;
    XYZtoSRGB(totalX, totalY, totalZ, r, g, b);

    return Colour(jlimit(0, 255, (int)(r * 255.0f)),
                  jlimit(0, 255, (int)(g * 255.0f)),
                  jlimit(0, 255, (int)(b * 255.0f)));
}

// ===== DMX Level Computation for Color Output =====

bool ColorEngine::computeEmitterDMXLevels(SubFixture* sf,
                                           float r, float g, float b,
                                           float intensity,
                                           HashMap<SubFixtureChannel*, float>& outLevels,
                                           bool constantLuminance)
{
    Array<EmitterCalibration> emitters = getCalibrations(sf);
    if (emitters.size() < 3) return false;  // Not enough calibration data

    // Convert target sRGB to CIE XYZ (normalized)
    float X, Y, Z;
    sRGBtoXYZ(r, g, b, X, Y, Z);

    // Solve for emitter weights that reproduce this chromaticity
    Array<float> weights;
    solveEmitterWeights(emitters, X, Y, Z, weights);

    // Normalize weights according to the caller's chosen mode
    normalizeWeights(emitters, weights, intensity, constantLuminance);

    // Convert weights to DMX levels and populate result
    for (int i = 0; i < emitters.size() && i < weights.size(); i++) {
        float dmxLevel = weightToDMXLevel(emitters[i], weights[i]);
        if (emitters[i].channel != nullptr) {
            outLevels.set(emitters[i].channel, dmxLevel);
        }
    }

    return true;
}
