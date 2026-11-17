#pragma once
#include <JuceHeader.h>
#include "BoomDSP.h"
#include "FactoryLibrary.h"
#include <atomic>
#include <vector>

class BoomAudioProcessor final : public juce::AudioProcessor {
public:
 BoomAudioProcessor();
 ~BoomAudioProcessor() override = default;
 void prepareToPlay(double sampleRate, int samplesPerBlock) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "BOOM";}
 bool acceptsMidi() const override {return false;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 0.0;}
 int getNumPrograms() override {return 100;}
 int getCurrentProgram() override {return std::max(0,selectedPreset.load());}
 void setCurrentProgram(int n) override {loadFactoryPreset(n);}
 const juce::String getProgramName(int n) override {return boom::makePreset(n).name;}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void* data,int size) override;
 void loadFactoryPreset(int index);
 void loadStyle(int index);
 void requestLoudnessReset() noexcept {loudnessResetRequested.store(true);}
 void storeComparisonState(int slot);
 void recallComparisonState(int slot);
 void copyComparisonState(int sourceSlot,int destinationSlot);
 bool isFavorite(int index) const;
 void toggleFavorite(int index);
 bool isStyleFavorite(int index) const;
 void toggleStyleFavorite(int index);
 // UI-thread user library. Each preset contains a full parameter snapshot.
 int saveUserPreset(const juce::String& name);
 bool renameUserPreset(int index,const juce::String& name);
 bool deleteUserPreset(int index);
 bool loadUserPreset(int index);
 int userPresetCount() const;
 juce::String userPresetName(int index) const;
 juce::String userPresetTags(int index) const;
 bool setUserPresetTags(int index,const juce::String& tags);
 bool exportUserPreset(int index,const juce::File& destination) const;
 bool importUserPreset(const juce::File& source);

 juce::AudioProcessorValueTreeState& state() noexcept {return apvts;}
 const juce::AudioProcessorValueTreeState& state() const noexcept {return apvts;}
 boom::Metering lastMeters() const noexcept;
 int presetIndex() const noexcept {return selectedPreset.load();}
 int styleIndex() const noexcept {return selectedStyle.load();}
 static juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout();
 void writeParameters(const boom::Parameters& p);
private:
 juce::AudioProcessorValueTreeState apvts;
 boom::Engine engine;
 std::atomic<int> selectedPreset{-1},selectedStyle{-1};
 std::atomic<bool> loudnessResetRequested{false};
 mutable juce::CriticalSection metadataLock;
 std::array<juce::ValueTree,2> comparisonSlots;
 std::array<bool,100> favorites{};
 std::array<bool,40> styleFavorites{};
 struct UserPreset {juce::String name;juce::ValueTree snapshot;juce::String tags;};
 std::vector<UserPreset> userPresets;
 void applySnapshot(const juce::ValueTree& snapshot);

 std::atomic<float> inPeakL{0},inPeakR{0},outPeakL{0},outPeakR{0};
 std::atomic<float> vuL{0},vuR{0},grComp{0},grLimit{0},rmsL{0},rmsR{0};
 // These six meters were used in PluginProcessor.cpp but were absent from the
 // v0.2 declaration; that would prevent a JUCE plugin from compiling.
 std::atomic<float> momentaryLufs{-100},shortTermLufs{-100},integratedLufs{-100};
 std::atomic<float> lra{0},truePeak{0},correlation{0};
 boom::Parameters readParameters() const noexcept;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomAudioProcessor)
};
