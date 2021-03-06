#include "system/fileoutput.h"

FileOutput::FileOutput( const std::vector<std::string> filenames, const Parameters &p, const OperatorMatrices &op ) {
    Log::L2( "Creating FileOutputs... " );
    long unsigned int config_maximumNumberOfFiles = 3;
    if ( filenames.size() != config_maximumNumberOfFiles ) {
        Log::L2( "WARNING: Number of input elements mismatches maximum files... " );
    }
    output_no_dm = p.output_no_dm;
    if ( !output_no_dm ) {
        fp_densitymatrix = std::fopen( ( p.subfolder + filenames.at( 0 ) ).c_str(), "w" );
        if ( !fp_densitymatrix )
            Log::L2( "\nCould not open file for densitymatrix!\n" );
        else {
            fmt::print( fp_densitymatrix, "t\t" );
            if ( p.output_full_dm ) {
                for ( int i = 0; i < p.maxStates; i++ )
                    for ( int j = 0; j < p.maxStates; j++ ) {
                        fmt::print( fp_densitymatrix, "|{}><{}|\t", op.base.at( i ), op.base.at( j ) );
                    }
            } else
                for ( int i = 0; i < p.maxStates; i++ )
                    fmt::print( fp_densitymatrix, "|{}><{}|\t", op.base.at( i ), op.base.at( i ) );
            fmt::print( fp_densitymatrix, "\n" );
        }
    }
    fp_atomicinversion = std::fopen( ( p.subfolder + filenames.at( 1 ) ).c_str(), "w" );
    if ( !fp_atomicinversion )
        Log::L2( "\nCould not open file for atomic inversion!\n" );
    else
        fmt::print( fp_atomicinversion, "t\t|G><G|\t|X_H><X_H|\t|X_V><X_V|\t|B><B|\n" );
    fp_photonpopulation = std::fopen( ( p.subfolder + filenames.at( 2 ) ).c_str(), "w" );
    if ( !fp_photonpopulation )
        Log::L2( "\nCould not open file for photonpopulation!\n" );
    else
        fmt::print( fp_photonpopulation, "t\tHorizontal\tVertical\tEmission-Probability-H\tEmission-Probability-V{}{}\n", ( p.numerics_output_raman_population ? "\tRaman-Population-H\tRaman-Poppulation-V\tRaman-Emission-Probability-H\tRaman-Emission-Probability-V" : "" ), ( p.numerics_output_electronic_emission ? "\tElectronic-H\tElectronic-V" : "" ) );
    Log::L2( "done!\n" );
}

void FileOutput::close() {
    Log::L2( "Closing file outputs..." );
    if ( !output_no_dm ) {
        std::fclose( fp_densitymatrix );
    }
    std::fclose( fp_atomicinversion );
    std::fclose( fp_photonpopulation );
    Log::L2( "Done!\n" );
}