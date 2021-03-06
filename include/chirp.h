#pragma once
#include "global.h"
#include "misc/interpolant.h"
#include "system/parameter.h"

class Chirp {
   public:
    class Inputs {
       public:
        double t_start, t_end, t_step;
        std::vector<double> t, y, ddt;
        std::string type;
        bool isSineChirp = false;
        int order;
        Inputs() {}
        Inputs( double t_start, double t_end, double t_step, std::string type, int order ) : t_start( t_start ), t_end( t_end ), t_step( t_step ), type( type ), order( order ) {}
        void add( double _t, double _y, double _ddt );
        void add( std::vector<Parameter> &_t, std::vector<Parameter> &_y, std::vector<Parameter> &_ddt );
    };

   private:
    // Chirp values
    std::vector<double> chirparray;
    std::vector<double> chirparray_derivative;
    std::vector<double> chirparray_integral;
    // Saving the corresponding t
    std::vector<double> timearray;
    // Helper variables/functions
    int size;
    int counter_evaluated, counter_returned;
    std::vector<double> steps;
    Interpolant interpolant;
    Inputs inputs;

   public:
    Chirp(){};
    Chirp( Inputs &inputs );
    void generate();
    void fileOutput( std::string filepath );
    double evaluate( double t );
    double evaluate_derivative( double t );
    double evaluate_integral( double t );
    double get( double t, bool force_evaluate = false );
    double derivative( double t, bool force_evaluate = false );
    double integral( double t, bool force_evaluate = false );
    void log();
};