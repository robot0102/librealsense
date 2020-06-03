// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp

// We have our own main
#define NO_CATCH_CONFIG_MAIN

#include "d2rgb-common.h"


template< typename T >
void read_binary_file( char const * dir, char const * bin, T * data )
{
    std::string filename = dir;
    filename += "\\";
    filename += bin;
    AC_LOG( DEBUG, "... " << filename );
    std::fstream f = std::fstream( filename, std::ios::in | std::ios::binary );
    if( ! f )
        throw std::runtime_error( "failed to read file:\n" + filename );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb != sizeof( T ) )
        throw std::runtime_error( to_string()
            << "file size (" << cb << ") does not match data size (" << sizeof(T) << "): " << filename );
    std::vector< T > vec( cb / sizeof( T ));
    f.read( (char*) data, cb );
    f.close();
}

int main( int argc, char * argv[] )
{
    bool ok = true;
    // Each of the arguments is the path to a directory to simulate
    // We skip argv[0] which is the path to the executable
    // We don't complain if no arguments -- that's how we'll run as part of unit-testing
    for( int i = 1; i < argc; ++i )
    {
        try
        {
            char const * dir = argv[i];
            std::cout << "Processing: " << dir << " ..." << std::endl;

            algo::calib calibration;
            read_binary_file( dir, "rgb.calib", &calibration );

            camera_params camera;
            camera.rgb = calibration.get_intrinsics();
            camera.extrinsics = calibration.get_extrinsics();
            algo::rs2_intrinsics_double d_intr;  // intrinsics written in double!
            read_binary_file( dir, "depth.intrinsics", &d_intr );
            camera.z = d_intr;
            read_binary_file( dir, "depth.units", &camera.z_units );
            read_binary_file( dir, "cal.info", &camera.cal_info );
            read_binary_file( dir, "cal.registers", &camera.cal_regs );
            read_binary_file( dir, "dsm.params", &camera.dsm_params );

            algo::optimizer cal;
            init_algo( cal, dir, "\\rgb.raw", "\\rgb_prev.raw", "\\ir.raw", "\\depth.raw", camera );

            TRACE( "\n___\nis_scene_valid" );
            if( !cal.is_scene_valid() )
            {

            }

            TRACE( "\n___\noptimize" );
            size_t n_iteration = cal.optimize(
                []( algo::iteration_data_collect const & data )
                {
                } );

            TRACE( "\n___\nis_valid_results" );
            if( !cal.is_valid_results() )
            {

            }
            TRACE( "\n___\nRESULTS:" );

            auto intr = cal.get_calibration().get_intrinsics();
            auto extr = cal.get_calibration().get_extrinsics();
            AC_LOG( DEBUG, AC_D_PREC
                << "intr[ "
                << intr.width << "x" << intr.height
                << "  ppx: " << intr.ppx << ", ppy: " << intr.ppy << ", fx: " << intr.fx
                << ", fy: " << intr.fy << ", model: " << int( intr.model ) << " coeffs["
                << intr.coeffs[0] << ", " << intr.coeffs[1] << ", " << intr.coeffs[2]
                << ", " << intr.coeffs[3] << ", " << intr.coeffs[4] << "] ]" );
            AC_LOG( DEBUG, AC_D_PREC << "extr" << (rs2_extrinsics) extr );
            AC_LOG( DEBUG, AC_D_PREC << "dsm" << cal.get_dsm_params() );
        }
        catch( std::exception const & e )
        {
            std::cerr << "\n___\ncaught exception: " << e.what() << std::endl;
            ok = false;
        }
        catch( ... )
        {
            std::cerr << "\n___\ncaught unknown exception!" << std::endl;
            ok = false;
        }
    }
    
    return ! ok;
}

