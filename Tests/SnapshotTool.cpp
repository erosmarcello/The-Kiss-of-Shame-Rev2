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

    bool extreme = false;
    for (int i = 2; i < argc; ++i)
        if (juce::String(argv[i]) == "--extreme")
            extreme = true;

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

        const auto file = outDir.getChildFile(juce::String("KissOfShame_") + eraName
                                              + (extreme ? "_extreme" : "") + ".png");
        file.deleteFile();
        juce::FileOutputStream stream(file);
        if (stream.openedOk())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream(snapshot, stream);
            std::printf("wrote %s (%dx%d)\n", file.getFullPathName().toRawUTF8(),
                        snapshot.getWidth(), snapshot.getHeight());
        }
    }

    return 0;
}
