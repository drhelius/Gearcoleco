/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2021 Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, GearcolecoButton)
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

@interface GearcolecoEmulator : NSObject

@property (nonatomic, readonly, getter=isLoaded) BOOL loaded;
@property (nonatomic, readonly, getter=isPaused) BOOL paused;
@property (nonatomic, getter=isMuted) BOOL muted;
@property (nonatomic, readonly) const uint16_t* frameBuffer;
@property (nonatomic, readonly) NSInteger frameWidth;
@property (nonatomic, readonly) NSInteger frameHeight;
@property (nonatomic, readonly) double framesPerSecond;

+ (nullable NSString*)romCRCInArchiveAtURL:(NSURL*)url NS_SWIFT_NAME(romCRC(inArchiveAt:));
- (void)configureWithTiming:(NSInteger)timing
                     mapper:(NSInteger)mapper
                  videoChip:(NSInteger)videoChip
                    palette:(NSInteger)palette
                   overscan:(NSInteger)overscan
              noSpriteLimit:(BOOL)noSpriteLimit
              saveStateSlot:(NSInteger)saveStateSlot
          firmwareDirectory:(NSURL*)firmwareDirectory
    NS_SWIFT_NAME(configure(timing:mapper:videoChip:palette:overscan:noSpriteLimit:saveStateSlot:firmwareDirectory:));
- (BOOL)loadROMAtURL:(NSURL*)url error:(NSError* _Nullable* _Nullable)error NS_SWIFT_NAME(loadROM(at:));
- (void)runFrame;
- (void)setButton:(GearcolecoButton)button pressed:(BOOL)pressed;
- (void)releaseAllButtons;
- (void)pause;
- (void)resume;
- (void)reset;
- (void)saveRAM;
- (void)saveState;
- (void)loadState;
- (void)startAudio;
- (void)stopAudio;

@end

NS_ASSUME_NONNULL_END
