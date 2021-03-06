#pragma once
// Dependencies
#include "global.h"
#include "misc/helperfunctions.h"
#include "misc/log.h"
#include "misc/timer.h"
#include "system/parameter.h"

#define GLOBAL_PROGRAM_VERSION "2.1.2"
#define GLOBAL_PROGRAM_LASTCHANGE "Sparse Pathintegral"

class Parameters {
   public:
    // Numerical Parameters
    Parameter hbar, kb;
    std::string subfolder;
    bool numerics_calculate_spectrum, numerics_calculate_g2, numerics_use_simplified_g2, numerics_use_interactionpicture, numerics_use_rwa;
    // Also output electronic emission probabilities
    bool numerics_output_electronic_emission;
    int numerics_order_timetrafo;
    int numerics_order_t, numerics_order_tau, numerics_order_highest;
    int output_advanced_log, output_handlerstrings, output_operators, output_coefficients;
    int iterations_t_skip, iterations_tau_resolution, iterations_w_resolution;
    int iterations_wtau_skip;
    bool scale_parameters;
    double scale_value;
    // Runtime parameters and other stuff
    int iterations_t_max;
    std::vector<double> trace;
    bool output_full_dm, output_no_dm;
    int numerics_maximum_threads, numerics_phonon_approximation_markov1, numerics_phonon_approximation_order;
    double akf_deltaWmax;
    Parameter spectrum_frequency_center, spectrum_frequency_range;
    bool numerics_calculate_spectrum_H, numerics_calculate_spectrum_V;
    bool numerics_calculate_g2_H, numerics_calculate_g2_V, numerics_calculate_g2_C;
    bool numerics_use_saved_coefficients;
    bool numerics_output_raman_population;
    bool numerics_calculate_timeresolution_indistinguishability;
    int numerics_phonons_maximum_threads;
    bool numerics_use_saved_hamiltons;
    long unsigned int numerics_saved_coefficients_cutoff; // True: Only save last few coefficients (only viable for T-direction, not for G1/2)
    long unsigned int numerics_saved_coefficients_max_size;
    int logfilecounter;
    bool numerics_stretch_correlation_grid, numerics_interpolate_outputs;

    // Path Integral Numerics
    double numerics_pathintegral_stepsize_iterator;      // = 1E-12;
    double numerics_pathintegral_squared_threshold;      // = 1E-32;
    double numerics_pathintegral_sparse_prune_threshold; // = 1E-1;
    // Propagator Cutoff; When iterated multiple times, M(t0->t1) may gain additional entries in between the RK iterations from t0 to t1. When set to true, the final non-zero matrix entries will be mapped onto the non-zero entries after
    // the first iteration, meaning any additional non-zero entries besides the ones created within the first iteration are lost.
    bool numerics_pathintegral_docutoff_propagator; //=false
    // Dynamic Cutoff; While true, the squared threshold will be increased or decreased until the number of ADM elements is approximately equal to the cutoff iterations set.
    long long numerics_pathintegral_dynamiccutoff_iterations_max; //=0

    // System Parameters
    // Time variables
    Parameter t_start, t_end, t_step, t_step_pathint;
    // System Dimensions
    int maxStates, p_max_photon_number;
    // Starting state:
    int p_initial_state, p_initial_state_photon_h, p_initial_state_photon_v;
    std::string p_initial_state_electronic;
    bool startCoherent;

    // Non mandatory parameters, dependant on system chosen:
    // System Parameterss
    Parameter p_omega_atomic_G_V;
    Parameter p_omega_atomic_G_H;
    Parameter p_omega_atomic_V_B;
    Parameter p_omega_atomic_H_B;
    Parameter p_omega_atomic_B;
    Parameter p_omega_cavity_V;
    Parameter p_omega_cavity_H;
    Parameter p_omega_coupling;
    Parameter p_omega_cavity_loss;
    Parameter p_omega_pure_dephasing;
    Parameter p_omega_decay;
    Parameter p_phonon_b, p_phonon_alpha, p_phonon_wcutoff, p_phonon_T, p_phonon_tcutoff, p_phonon_pure_dephasing;
    bool p_phonon_adjust;
    int p_phonon_nc;
    Parameter p_deltaE;
    Parameter p_biexciton_bindingenergy;

    // Calculated System properties:
    Parameter init_detuning_G_H, init_detuning_G_V, init_detuning_H_B, init_detuning_V_B, max_detuning_G_H, max_detuning_G_V, max_detuning_H_B, max_detuning_V_B;
    Parameter init_rabifrequenz_G_H, init_rabifrequenz_G_V, init_rabifrequenz_H_B, init_rabifrequenz_V_B, max_rabifrequenz_G_H, max_rabifrequenz_G_V, max_rabifrequenz_H_B, max_rabifrequenz_V_B;
    Parameter init_detuning, max_detuning, init_rabifrequenz, max_rabifrequenz;

    // Chirp and Pulse properties:
    std::vector<Parameter> pulse_center, pulse_amp, pulse_omega, pulse_sigma, pulse_omega_chirp;
    std::vector<std::string> pulse_type;
    std::vector<std::string> pulse_pol;
    double chirp_total;
    std::vector<Parameter> chirp_t, chirp_y, chirp_ddt;
    std::string chirp_type;

    // Constructor
    Parameters(){};
    Parameters( const std::vector<std::string> &arguments );

    // Log function; Uses log subclass (log.h)
    // @param &info: Additional information this class does not have access too when created, e.g. basis
    void log( const std::vector<std::string> &info );

    // Help function, output when --help is called
    static void help();

    // Parses the input from a string input vector. Uses the Parse_Parameters class from misc/commandlinearguments.h
    // @param &arguments: Vector of string arguments, e.g. from argv
    // @return Returns true if parsing was successful
    bool parseInput( const std::vector<std::string> &arguments );

    // Adjusting inputs
    // @return Returns true if successful
    bool adjustInput();

    // Scale inputs. Has to be called before anything else is calculated //TODO: finalize
    // @param scaling: Value to scale with, e.g. 1E12 for ps
    // @return Returns true if scaling was successful
    bool scaleInputs( const double scaling );

    // Scale single input
    // @param variable: Numerical value of variable to scale
    // @param scaling: Value to scale with, e.g. 1E12 for ps
    // @return Returns variable*scaling
    template <typename T>
    T scaleVariable( const T variable, const double scaling ) {
        if ( scale_parameters ) {
            return variable * scaling;
        }
        return variable;
    }

    // Converts a named state vector, e.g. |g,0,0> into a matrix index
    // @param mode: Element of G,H,V,E
    // @param state: Photon number, 0,1,...
    // @return Returns matrix index corresponding to |X,n,m>
    int index_to_state( const char mode = 'E', const int state = 0 );

    // Approximates the ideal timestep for the initial system parameters chosen
    // @return Returns ideal timestep
    double getIdealTimestep();
};