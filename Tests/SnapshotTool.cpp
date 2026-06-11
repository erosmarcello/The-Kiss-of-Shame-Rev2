// Renders the editor to PNG files, one per era — headless screenshot tool
// for docs, visual review, and eyeballing GUI regressions.
//
//   KissOfShameSnapshot <outputDirectory> [--extreme]

#include <PluginProcessor.h>
#include <PluginEditor.h>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outDir(argc > 1 ? juce::String(argv[1])
                                     : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    bool extreme = false, picker = false;
    for (int i = 2; i < argc; ++i)
    {
        if (juce::String(argv[i]) == "--extreme")
            extreme = true;
        if (juce::String(argv[i]) == "--picker")
            picker = true;
    }

    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    proc->setPlayConfigDetails(2, 2, 48000.0, 512);
    proc->prepareToPlay(48000.0, 512);

    auto& kos = dynamic_cast<KissOfShameAudioProcessor&>(*proc);
    kos.setShameExtreme(extreme);

    // a representative pose for the faceplate
    auto pose = [&kos](const char* id, float v)
    {
        if (auto* p = kos.apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(v));
    };
    pose(ParamIDs::shame, 0.62f);
    pose(ParamIDs::hiss, 0.3f);
    pose(ParamIDs::age, 0.45f);
    pose(ParamIDs::environment, 4.0f);

    for (auto* eraName : { "heritage", "modern" })
    {
        kos.setUIEra(eraName);

        std::unique_ptr<juce::AudioProcessorEditor> editor(proc->createEditorIfNeeded());
        if (editor == nullptr)
            return 1;

        juce::MessageManager::getInstance()->runDispatchLoopUntil(250);

        auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());

        // The CallOutBox needs a native peer, which headless snapshots lack —
        // composite the real Picker component at its callout position instead.
        if (picker)
        {
            const auto era = juce::String(eraName) == "modern" ? UIEra::modern : UIEra::heritage;
            EnvironmentsComponent::Picker pickerComp(era, 4, 0.45f, nullptr);
            auto pickerImage = pickerComp.createComponentSnapshot(pickerComp.getLocalBounds());

            juce::Graphics g(snapshot);
            const int px = 388 + (183 - pickerComp.getWidth()) / 2;
            const int py = 654 - pickerComp.getHeight() - 10;
            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.fillRoundedRectangle((float) px - 3, (float) py - 3,
                                   (float) pickerComp.getWidth() + 6, (float) pickerComp.getHeight() + 6, 9.0f);
            g.setOpacity(1.0f);
            g.drawImageAt(pickerImage, px, py);
        }

        const auto file = outDir.getChildFile(juce::String("KissOfShame_") + eraName
                                              + (extreme ? "_extreme" : "")
                                              + (picker ? "_picker" : "") + ".png");
        file.deleteFile();
        juce::FileOutputStream stream(file);
        if (stream.openedOk())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream(snapshot, stream);
            std::printf("wrote %s (%dx%d)\n", file.getFullPathName().toRawUTF8(),
                        snapshot.getWidth(), snapshot.getHeight());
        }

        editor->removeFromDesktop();
    }

    return 0;
}
