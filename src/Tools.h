#pragma once

#include "Singleton.hpp"

class Tools final : public Singleton<Tools>
{
    public:
        explicit Tools(typename Singleton<Tools>::token) {};

        wxString getFullFileName( const std::wstring& filename ) const;
        std::shared_ptr<wxBitmap> loadBitmapFromFile( const std::wstring& filename ) const;
        std::vector< std::vector<int>> loadLevelFromFile( const std::wstring& filename, unsigned short  levelNum ) const;

        void bhmLine( std::vector<wxPoint>& trajectory, int x1, int y1, int x2, int y2, const wxSize& limits ) const;

    protected:
}; 
