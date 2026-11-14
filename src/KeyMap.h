/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2021  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#ifndef KEY_MAP_H
#define KEY_MAP_H

#include <string>
#include "definitions.h"

#define GC_HOST_KEY_COUNT 0x200
#define GC_KEY_MAP_TARGETS 4

typedef void (*GC_KeyEventFn)(void* context, GC_AdamKey key, bool pressed);

enum GC_KeyMapLayer
{
    GC_KEY_MAP_BUILT_IN = 0,
    GC_KEY_MAP_GAME,
    GC_KEY_MAP_USER,
    GC_KEY_MAP_LAYER_COUNT
};

struct GC_KeyMapEntry
{
    bool set;
    u8 count;
    u8 keys[GC_KEY_MAP_TARGETS];
};

class KeyMap
{
public:
    KeyMap();
    void Init(GC_KeyEventFn callback, void* context);
    void HostKeyPressed(u16 usage);
    void HostKeyReleased(u16 usage);
    void IgnoreHostKey(u16 usage);
    void DiscardDeferredHostKeys();
    bool IsHostKeyDown(u16 usage) const;
    void ReleaseAll();
    void Suspend();
    void Resync();
    void SetBlocked(bool blocked);
    bool IsBlocked() const;
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void ResetBuiltIn(bool specials = true);
    bool SetLayer(GC_KeyMapLayer layer, const char* text, std::string* error);
    std::string GetLayer(GC_KeyMapLayer layer) const;
    std::string GetMerged() const;
    void ClearLayer(GC_KeyMapLayer layer);
    bool IsLayerEmpty(GC_KeyMapLayer layer) const;
    void SetEntry(GC_KeyMapLayer layer, u16 usage, const GC_KeyMapEntry& entry);
    const GC_KeyMapEntry& GetLayerEntry(GC_KeyMapLayer layer, u16 usage) const;
    const GC_KeyMapEntry& GetEntry(u16 usage, GC_KeyMapLayer* layer = NULL) const;
    bool Bind(u16 usage, const GC_AdamKey* keys, int count);
    bool Unbind(u16 usage, GC_AdamKey key);
    bool RestoreBuiltIn(u16 usage);
    bool IsChanged() const;
    void SetChanged(bool changed);

    static bool IsHostKey(u16 usage);
    static int FindHostKey(const char* name);
    static const char* GetHostKeyName(u16 usage);
    static const char* GetHostKeyLabel(u16 usage);
    static int FindKey(const char* name);
    static const char* GetKeyName(GC_AdamKey key);
    static const char* GetKeyLabel(GC_AdamKey key);
    static int GetDefaultKey(u16 usage);
    static bool ParseEntry(const std::string& host, const std::string& value, u16* usage, GC_KeyMapEntry* entry,
        std::string* error);
    static std::string FormatTargets(const GC_KeyMapEntry& entry);

private:
    const GC_KeyMapEntry& GetActiveEntry(u16 usage) const;
    const GC_KeyMapEntry& GetBaseEntry(GC_KeyMapLayer layer, u16 usage) const;
    void SetUserEntry(u16 usage, const GC_KeyMapEntry& entry);
    void PressHostTargets(u16 usage);
    void FlushDeferredHostKeys();
    static bool IsDeferredHostKey(u16 usage);
    void PressKey(u8 key);
    void ReleaseKey(u8 key);
    void SyncKey(u8 key);
    static bool IsModifier(u8 key);
    static bool SameTargets(const GC_KeyMapEntry& a, const GC_KeyMapEntry& b);
    std::string FormatLayers(int first, int last) const;

private:
    GC_KeyMapEntry m_Layers[GC_KEY_MAP_LAYER_COUNT][GC_HOST_KEY_COUNT];
    GC_KeyMapEntry m_HostTargets[GC_HOST_KEY_COUNT];
    bool m_HostDown[GC_HOST_KEY_COUNT];
    bool m_HostDeferred[GC_HOST_KEY_COUNT];
    int m_References[GC_ADAM_KEY_COUNT];
    bool m_bEnabled;
    bool m_bBlocked;
    bool m_bChanged;
    GC_KeyEventFn m_Callback;
    void* m_pContext;
};

#endif /* KEY_MAP_H */
