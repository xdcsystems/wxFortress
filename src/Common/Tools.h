#pragma once

#include "Singleton.hpp"

// Forward declarations
class wxBitmap;
class wxPoint;
class wxSize;

class Tools final : public Singleton<Tools>
{
    public:
        explicit Tools(typename Singleton<Tools>::token) {};

        std::string getFullFileName( const std::string& filename ) const;
        std::shared_ptr<wxBitmap> loadBitmapFromFile( const std::string& filename ) const;
        std::vector< std::vector<int>> loadLevelFromFile( const std::string& filename, unsigned short  levelNum ) const;

        void bhmLine( std::vector<wxPoint>& trajectory, int x1, int y1, int x2, int y2, const wxSize& limits ) const;

    protected:
}; 
