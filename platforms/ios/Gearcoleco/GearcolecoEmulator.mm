/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2021 Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#import "GearcolecoEmulator.h"

#import <AVFAudio/AVFAudio.h>

#include <string.h>
#include <strings.h>

#include "IOSAudioQueue.h"

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"
#undef MINIZ_NO_ZLIB_COMPATIBLE_NAMES

#undef MIN
#undef MAX
#include "../../../src/gearcoleco.h"

bool g_mcp_stdio_mode = false;

static NSString* const GearcolecoEmulatorErrorDomain = @"me.ignaciosanchez.gearcoleco.emulator";

static bool IsROMArchiveEntry(const char* filename)
{
    const char* extension = strrchr(filename, '.');
    if (!extension)
        return false;

    return (strcasecmp(extension, ".col") == 0) ||
           (strcasecmp(extension, ".cv") == 0) ||
           (strcasecmp(extension, ".bin") == 0) ||
           (strcasecmp(extension, ".rom") == 0);
}

static NSString* ROMCRCInArchive(NSURL* url)
{
    if (!url.isFileURL)
        return nil;

    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if (!mz_zip_reader_init_file(&archive, url.fileSystemRepresentation, 0))
        return nil;

    NSString* result = nil;
    mz_uint fileCount = mz_zip_reader_get_num_files(&archive);
    for (mz_uint index = 0; index < fileCount; index++)
    {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&archive, index, &fileStat))
            break;
        if (!IsROMArchiveEntry(fileStat.m_filename))
            continue;

        size_t size = 0;
        void* data = mz_zip_reader_extract_to_heap(&archive, index, &size, 0);
        if (!data)
            break;

        mz_ulong checksum = mz_crc32(MZ_CRC32_INIT, (const unsigned char*)data, size);
        free(data);
        result = [NSString stringWithFormat:@"%08X", (unsigned int)checksum];
        break;
    }

    mz_zip_reader_end(&archive);
    return result;
}

static Cartridge::CartridgeRegions TimingForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return Cartridge::CartridgeNTSC;
        case 2:
            return Cartridge::CartridgePAL;
        default:
            return Cartridge::CartridgeUnknownRegion;
    }
}

static Cartridge::CartridgeTypes MapperForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return Cartridge::CartridgeColecoVision;
        case 2:
            return Cartridge::CartridgeMegaCart;
        case 3:
            return Cartridge::CartridgeActivisionCart;
        case 4:
            return Cartridge::CartridgeOCM;
        default:
            return Cartridge::CartridgeNotSupported;
    }
}

static Video::Overscan OverscanForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return Video::OverscanTopBottom;
        case 2:
            return Video::OverscanFull284;
        case 3:
            return Video::OverscanFull320;
        default:
            return Video::OverscanDisabled;
    }
}

static GC_VideoChip VideoChipForOption(NSInteger option)
{
    switch (option)
    {
        case 1:
            return GC_VIDEO_CHIP_TMS9918A;
        case 2:
            return GC_VIDEO_CHIP_F18A;
        default:
            return GC_VIDEO_CHIP_AUTO;
    }
}

@interface GearcolecoEmulator ()
{
    GearcolecoCore* m_core;
    u16* m_frameBuffer;
    s16* m_audioBuffer;
    IOSAudioQueue m_audioQueue;
    uint32_t m_pressedButtons;
    BOOL m_loaded;
    BOOL m_muted;
    BOOL m_noSpriteLimit;
    NSInteger m_overscan;
    NSInteger m_videoChip;
    NSInteger m_palette;
    NSInteger m_saveStateSlot;
    NSInteger m_frameWidth;
    NSInteger m_frameHeight;
    double m_framesPerSecond;
    Cartridge::ForceConfiguration m_configuration;
    NSURL* m_firmwareDirectory;
    AVAudioEngine* m_audioEngine;
    AVAudioSourceNode* m_audioSourceNode;
}

- (void)applyConfiguration;
- (BOOL)loadFirmware;
- (void)updateRuntimeInfo;
- (void)configureAudio;
- (void)audioEngineConfigurationChanged:(NSNotification*)notification;
- (void)clearAudio;
- (void)enqueueAudioSamples:(const s16*)samples count:(int)count;
- (OSStatus)renderAudioFrames:(AVAudioFrameCount)frameCount outputData:(AudioBufferList*)outputData silence:(BOOL*)isSilence;

@end

@implementation GearcolecoEmulator

+ (NSString*)romCRCInArchiveAtURL:(NSURL*)url
{
    return ROMCRCInArchive(url);
}

- (instancetype)init
{
    self = [super init];

    if (self)
    {
        m_core = new GearcolecoCore();
        m_core->Init(GC_PIXEL_RGB565);

        m_frameBuffer = new u16[GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT]();
        m_audioBuffer = new s16[GC_AUDIO_BUFFER_SIZE]();
        m_audioQueue.Configure(GC_AUDIO_QUEUE_SIZE, 3);
        m_pressedButtons = 0;
        m_loaded = NO;
        m_muted = NO;
        m_noSpriteLimit = NO;
        m_overscan = 0;
        m_videoChip = 0;
        m_palette = 0;
        m_saveStateSlot = 1;
        m_frameWidth = GC_RESOLUTION_WIDTH;
        m_frameHeight = GC_RESOLUTION_HEIGHT;
        m_framesPerSecond = 60.0;
        m_configuration.type = Cartridge::CartridgeNotSupported;
        m_configuration.region = Cartridge::CartridgeUnknownRegion;

        [self applyConfiguration];
        [self configureAudio];
    }

    return self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [self stopAudio];

    if (m_loaded)
    {
        m_core->SaveRam();
    }

    SafeDeleteArray(m_audioBuffer);
    SafeDeleteArray(m_frameBuffer);
    SafeDelete(m_core);
}

- (void)configureWithTiming:(NSInteger)timing
                     mapper:(NSInteger)mapper
                  videoChip:(NSInteger)videoChip
                    palette:(NSInteger)palette
                   overscan:(NSInteger)overscan
              noSpriteLimit:(BOOL)noSpriteLimit
              saveStateSlot:(NSInteger)saveStateSlot
          firmwareDirectory:(NSURL*)firmwareDirectory
{
    m_configuration.type = MapperForOption(mapper);
    m_configuration.region = TimingForOption(timing);
    m_videoChip = videoChip;
    m_palette = (palette == 1) ? 1 : 0;
    m_overscan = overscan;
    m_noSpriteLimit = noSpriteLimit;
    m_firmwareDirectory = firmwareDirectory;

    if (saveStateSlot < 1)
    {
        m_saveStateSlot = 1;
    }
    else if (saveStateSlot > 5)
    {
        m_saveStateSlot = 5;
    }
    else
    {
        m_saveStateSlot = saveStateSlot;
    }

    [self applyConfiguration];
}

- (void)applyConfiguration
{
    m_core->SetVideoChip(VideoChipForOption(m_videoChip));
    m_core->GetVideo()->SetPredefinedPalette((int)m_palette);
    m_core->GetVideo()->SetOverscan(OverscanForOption(m_overscan));
    m_core->GetVideo()->SetNoSpriteLimit(m_noSpriteLimit);
}

- (BOOL)loadFirmware
{
    if (!m_firmwareDirectory)
    {
        return NO;
    }

    NSURL* biosURL = [m_firmwareDirectory URLByAppendingPathComponent:@"colecovision.rom"];
    if (![NSFileManager.defaultManager fileExistsAtPath:biosURL.path])
    {
        m_core->GetMemory()->UnloadBios();
        return NO;
    }

    m_core->GetMemory()->LoadBios(biosURL.fileSystemRepresentation);
    return m_core->GetMemory()->IsBiosLoaded();
}

- (void)updateRuntimeInfo
{
    GC_RuntimeInfo runtimeInfo;

    if (m_core->GetRuntimeInfo(runtimeInfo))
    {
        m_frameWidth = runtimeInfo.screen_width;
        m_frameHeight = runtimeInfo.screen_height;
        m_framesPerSecond = runtimeInfo.fps;
    }
}

- (BOOL)loadROMAtURL:(NSURL*)url error:(NSError**)error
{
    if (!url.isFileURL)
    {
        if (error)
        {
            *error = [NSError errorWithDomain:GearcolecoEmulatorErrorDomain
                                         code:1
                                     userInfo:@{NSLocalizedDescriptionKey: @"The selected item is not a local ROM file."}];
        }

        return NO;
    }

    if (m_loaded)
    {
        m_core->SaveRam();
    }

    [self releaseAllButtons];
    [self clearAudio];

    [self applyConfiguration];
    if (![self loadFirmware])
    {
        m_loaded = NO;

        if (error)
        {
            *error = [NSError errorWithDomain:GearcolecoEmulatorErrorDomain
                                         code:2
                                     userInfo:@{NSLocalizedDescriptionKey:
                                         @"ColecoVision BIOS not installed. Import colecovision.rom in Settings."}];
        }

        return NO;
    }

    BOOL loaded = m_core->LoadROM(url.fileSystemRepresentation, &m_configuration);

    if (!loaded)
    {
        m_loaded = NO;

        if (error)
        {
            *error = [NSError errorWithDomain:GearcolecoEmulatorErrorDomain
                                         code:3
                                     userInfo:@{NSLocalizedDescriptionKey: @"Gearcoleco could not load this ROM."}];
        }

        return NO;
    }

    m_core->LoadRam();
    [self applyConfiguration];
    m_core->Pause(false);
    m_loaded = YES;
    memset(m_frameBuffer, 0,
        GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT * sizeof(u16));
    [self updateRuntimeInfo];

    return YES;
}

- (void)runFrame
{
    if (!m_loaded || m_core->IsPaused())
    {
        return;
    }

    int sampleCount = 0;
    m_core->RunToVBlank(reinterpret_cast<u8*>(m_frameBuffer), m_audioBuffer, &sampleCount);
    [self updateRuntimeInfo];

    if (!m_muted && (sampleCount > 0))
    {
        [self enqueueAudioSamples:m_audioBuffer count:sampleCount];
    }
}

- (void)setButton:(GearcolecoButton)button pressed:(BOOL)pressed
{
    if (!m_loaded)
    {
        return;
    }

    uint32_t buttonMask = 1U << (uint32_t)button;
    BOOL wasPressed = (m_pressedButtons & buttonMask) != 0;

    if (pressed == wasPressed)
    {
        return;
    }

    GC_Keys key;

    switch (button)
    {
        case GearcolecoButtonUp:
            key = Key_Up;
            break;
        case GearcolecoButtonDown:
            key = Key_Down;
            break;
        case GearcolecoButtonLeft:
            key = Key_Left;
            break;
        case GearcolecoButtonRight:
            key = Key_Right;
            break;
        case GearcolecoButtonYellow:
            key = Key_Left_Button;
            break;
        case GearcolecoButtonRed:
            key = Key_Right_Button;
            break;
        case GearcolecoButtonBlue:
            key = Key_Blue;
            break;
        case GearcolecoButtonPurple:
            key = Key_Purple;
            break;
        case GearcolecoButtonKeypad0:
            key = Keypad_0;
            break;
        case GearcolecoButtonKeypad1:
            key = Keypad_1;
            break;
        case GearcolecoButtonKeypad2:
            key = Keypad_2;
            break;
        case GearcolecoButtonKeypad3:
            key = Keypad_3;
            break;
        case GearcolecoButtonKeypad4:
            key = Keypad_4;
            break;
        case GearcolecoButtonKeypad5:
            key = Keypad_5;
            break;
        case GearcolecoButtonKeypad6:
            key = Keypad_6;
            break;
        case GearcolecoButtonKeypad7:
            key = Keypad_7;
            break;
        case GearcolecoButtonKeypad8:
            key = Keypad_8;
            break;
        case GearcolecoButtonKeypad9:
            key = Keypad_9;
            break;
        case GearcolecoButtonKeypadAsterisk:
            key = Keypad_Asterisk;
            break;
        case GearcolecoButtonKeypadHash:
            key = Keypad_Hash;
            break;
    }

    if (pressed)
    {
        m_pressedButtons |= buttonMask;
        m_core->KeyPressed(Controller_1, key);
    }
    else
    {
        m_pressedButtons &= ~buttonMask;
        m_core->KeyReleased(Controller_1, key);
    }
}

- (void)releaseAllButtons
{
    if (!m_core)
    {
        return;
    }

    static const GearcolecoButton buttons[] =
    {
        GearcolecoButtonUp,
        GearcolecoButtonDown,
        GearcolecoButtonLeft,
        GearcolecoButtonRight,
        GearcolecoButtonYellow,
        GearcolecoButtonRed,
        GearcolecoButtonBlue,
        GearcolecoButtonPurple,
        GearcolecoButtonKeypad0,
        GearcolecoButtonKeypad1,
        GearcolecoButtonKeypad2,
        GearcolecoButtonKeypad3,
        GearcolecoButtonKeypad4,
        GearcolecoButtonKeypad5,
        GearcolecoButtonKeypad6,
        GearcolecoButtonKeypad7,
        GearcolecoButtonKeypad8,
        GearcolecoButtonKeypad9,
        GearcolecoButtonKeypadAsterisk,
        GearcolecoButtonKeypadHash
    };

    for (GearcolecoButton button : buttons)
    {
        if ((m_pressedButtons & (1U << (uint32_t)button)) != 0)
        {
            [self setButton:button pressed:NO];
        }
    }
}

- (void)pause
{
    if (m_loaded)
    {
        [self releaseAllButtons];
        m_core->Pause(true);
    }
}

- (void)resume
{
    if (m_loaded)
    {
        m_core->Pause(false);
    }
}

- (void)reset
{
    if (!m_loaded)
    {
        return;
    }

    [self releaseAllButtons];
    m_core->SaveRam();
    [self applyConfiguration];
    if (![self loadFirmware])
    {
        return;
    }
    m_core->ResetROMPreservingRAM(&m_configuration);
    [self applyConfiguration];
    [self updateRuntimeInfo];
    [self clearAudio];
}

- (void)saveRAM
{
    if (m_loaded)
    {
        m_core->SaveRam();
    }
}

- (void)saveState
{
    if (m_loaded)
    {
        m_core->SaveState(NULL, (int)m_saveStateSlot);
    }
}

- (void)loadState
{
    if (m_loaded)
    {
        [self releaseAllButtons];
        m_core->LoadState(NULL, (int)m_saveStateSlot);
        [self clearAudio];
    }
}

- (BOOL)isLoaded
{
    return m_loaded;
}

- (BOOL)isPaused
{
    return !m_loaded || m_core->IsPaused();
}

- (BOOL)isMuted
{
    return m_muted;
}

- (void)setMuted:(BOOL)muted
{
    m_muted = muted;

    if (muted)
    {
        [self clearAudio];
    }
}

- (const uint16_t*)frameBuffer
{
    return m_frameBuffer;
}

- (NSInteger)frameWidth
{
    return m_frameWidth;
}

- (NSInteger)frameHeight
{
    return m_frameHeight;
}

- (double)framesPerSecond
{
    return m_framesPerSecond;
}

- (void)configureAudio
{
    m_audioEngine = [[AVAudioEngine alloc] init];
    AVAudioFormat* format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:GC_AUDIO_SAMPLE_RATE channels:2];
    __weak GearcolecoEmulator* weakSelf = self;

    m_audioSourceNode = [[AVAudioSourceNode alloc] initWithFormat:format
                                                     renderBlock:^OSStatus(BOOL* isSilence,
                                                                         const AudioTimeStamp* timestamp,
                                                                         AVAudioFrameCount frameCount,
                                                                         AudioBufferList* outputData)
    {
        UNUSED(timestamp);
        GearcolecoEmulator* strongSelf = weakSelf;

        if (!strongSelf)
        {
            *isSilence = YES;

            for (UInt32 bufferIndex = 0; bufferIndex < outputData->mNumberBuffers; ++bufferIndex)
            {
                AudioBuffer* buffer = &outputData->mBuffers[bufferIndex];
                memset(buffer->mData, 0, buffer->mDataByteSize);
            }

            return noErr;
        }

        return [strongSelf renderAudioFrames:frameCount outputData:outputData silence:isSilence];
    }];

    [m_audioEngine attachNode:m_audioSourceNode];
    [m_audioEngine connect:m_audioSourceNode to:m_audioEngine.mainMixerNode format:format];
    [m_audioEngine prepare];

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(audioEngineConfigurationChanged:)
                                               name:AVAudioEngineConfigurationChangeNotification
                                             object:m_audioEngine];
}

- (void)startAudio
{
    if (m_audioEngine.isRunning)
    {
        return;
    }

    AVAudioSession* session = AVAudioSession.sharedInstance;
    NSError* error = nil;
    [session setCategory:AVAudioSessionCategoryAmbient
                    mode:AVAudioSessionModeDefault
                 options:AVAudioSessionCategoryOptionMixWithOthers
                   error:&error];

    if (!error)
    {
        NSError* preferenceError = nil;
        [session setPreferredSampleRate:GC_AUDIO_SAMPLE_RATE error:&preferenceError];
        preferenceError = nil;
        [session setPreferredIOBufferDuration:512.0 / GC_AUDIO_SAMPLE_RATE error:&preferenceError];
        [session setActive:YES error:&error];
    }

    [self clearAudio];

    if (!error)
    {
        [m_audioEngine startAndReturnError:&error];
    }

    if (error)
    {
        NSLog(@"Unable to start Gearcoleco audio: %@", error.localizedDescription);
    }
}

- (void)stopAudio
{
    if (m_audioEngine.isRunning)
    {
        [m_audioEngine pause];
    }

    [self clearAudio];

    NSError* error = nil;
    [AVAudioSession.sharedInstance setActive:NO
                                 withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                       error:&error];

    if (error)
    {
        NSLog(@"Unable to stop Gearcoleco audio: %@", error.localizedDescription);
    }
}

- (void)audioEngineConfigurationChanged:(NSNotification*)notification
{
    UNUSED(notification);
    [self clearAudio];

    __weak GearcolecoEmulator* weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        GearcolecoEmulator* strongSelf = weakSelf;

        if (!strongSelf || !strongSelf->m_loaded ||
            strongSelf->m_core->IsPaused() || strongSelf->m_audioEngine.isRunning)
        {
            return;
        }

        [strongSelf->m_audioEngine prepare];

        NSError* error = nil;
        [strongSelf->m_audioEngine startAndReturnError:&error];

        if (error)
        {
            NSLog(@"Unable to restart Gearcoleco audio: %@", error.localizedDescription);
        }
    });
}

- (void)clearAudio
{
    m_audioQueue.Reset();
}

- (void)enqueueAudioSamples:(const s16*)samples count:(int)count
{
    if (count > 0)
        m_audioQueue.Write(samples, (uint32_t)count);
}

- (OSStatus)renderAudioFrames:(AVAudioFrameCount)frameCount outputData:(AudioBufferList*)outputData silence:(BOOL*)isSilence
{
    bool audible = false;

    if (outputData->mNumberBuffers >= 2)
    {
        float* left = (float*)outputData->mBuffers[0].mData;
        float* right = (float*)outputData->mBuffers[1].mData;
        audible = m_audioQueue.Render(left, right, (uint32_t)frameCount);
    }
    else if (outputData->mNumberBuffers == 1)
    {
        float* output = (float*)outputData->mBuffers[0].mData;
        audible = m_audioQueue.RenderInterleaved(output, (uint32_t)frameCount);
    }

    *isSilence = !audible;
    return noErr;
}

@end
