#include "misc/commandlinearguments.h"

std::string CommandlineArguments::strip( std::string input, char lit ) {
    int a = input.find_first_of( lit ) + 1;
    int e = input.find_last_of( lit );
    // maybe: return "" if length is 0
    return input.substr( a, e - a );
}

std::vector<std::string> CommandlineArguments::strip( std::vector<std::string> input, char lit ) {
    std::vector<std::string> ret;
    for ( const auto& str : input ) ret.emplace_back( strip( str ) );
    return ret;
}

std::string CommandlineArguments::trail( std::string input, int totallength, std::string pre, int dir ) {
    if ( pre.size() == 0 )
        return input;
    int remaining = ( totallength - input.size() ) / pre.size();
    int trailing = ( totallength - input.size() ) % pre.size();
    for ( int i = 0; i < remaining; i++ )
        if ( dir == 0 )
            input = pre + input;
        else
            input = input + pre;
    if ( dir == 0 )
        input = pre.substr( 0, trailing ) + input;
    else
        input = input + pre.substr( 0, trailing );
    return input;
}

std::string CommandlineArguments::tail( std::string input, int totallength, std::string pre ) {
    return trail( input, totallength, pre, 1 );
}

bool CommandlineArguments::startswith( std::string input, std::string find ) {
    return input.substr( 0, find.size() ).compare( find ) == 0;
}

std::vector<std::string> CommandlineArguments::splitString( std::string input, std::string lit ) {
    std::vector<std::string> ret;
    long unsigned int i = 0;
    long unsigned int start = 0;
    while ( i < input.size() ) {
        if ( input.at( i ) == lit.at( 0 ) ) {
            bool legit = true;
            for ( long unsigned int j = 1; j < lit.size() && i + j < input.size(); j++ )
                if ( input.at( i + j ) != lit.at( j ) ) legit = false;
            if ( legit ) {
                ret.emplace_back( input.substr( start, i - start ) );
                start = i + lit.size();
            }
        }
        i++;
    }
    ret.emplace_back( input.substr( start ) );
    return ret;
}

void CommandlineArguments::init() {
    cla = CommandlineArguments();
}
void CommandlineArguments::init( int argc, char* argv[], std::string filepath ) {
    cla = CommandlineArguments( argc, argv, filepath );
}
void CommandlineArguments::init( const std::vector<std::string>& argv, std::string filepath ) {
    cla = CommandlineArguments( argv, filepath );
}

CommandlineArguments::CommandlineArguments CommandlineArguments::cla = CommandlineArguments();