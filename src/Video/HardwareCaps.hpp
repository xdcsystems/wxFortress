#pragma once

#include <algorithm>
#include <cstdint>

namespace HardwareCaps
{
    constexpr uint32_t kAmdVendorId = 0x1002u;

    // PCI device ID families for legacy ATI/AMD UVD and UVD+ (H.264 DPB limited to Level 4.1).
    inline bool isLegacyAmdUvd( uint32_t deviceId ) noexcept
    {
        static constexpr uint32_t kUvdFamilies[] = {
            0x9400u, // R600
            0x9440u, // R700 / RV770
            0x9450u, // RV770
            0x9460u, // RV770 / RV790
            0x94C0u, // RV610
            0x9500u, // RV670
            0x9510u, // RV670 / R680
            0x9580u, // RV630
            0x9590u, // RV635
            0x95C0u, // RV620 (HD 3400 series, e.g. Mobility Radeon HD 3470 0x95C4)
            0x9610u, // RS780
            0x9710u, // RS880
        };
        const uint32_t family = deviceId & 0xFFF0u;
        for ( uint32_t id : kUvdFamilies )
        {
            if ( id == family )
                return true;
        }
        return false;
    }

    inline int maxDpb41RefFrames( int width, int height ) noexcept
    {
        if ( width <= 0 || height <= 0 )
            return 4;
        const int64_t widthMbs = ( static_cast<int64_t>( width ) + 15 ) / 16;
        const int64_t heightMbs = ( static_cast<int64_t>( height ) + 15 ) / 16;
        const int64_t totalMbs = widthMbs * heightMbs;
        if ( totalMbs <= 0 )
            return 4;
        return static_cast<int>( ( std::min )( int64_t { 11 }, int64_t { 32768 } / totalMbs ) );
    }

    inline bool isLegacyAmdUvdStreamAllowed( int codecLevel, int refs, int width, int height ) noexcept
    {
        if ( codecLevel > 41 )
            return false;
        return refs <= maxDpb41RefFrames( width, height );
    }
} // namespace HardwareCaps
