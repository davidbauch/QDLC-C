#include "solver/solver_ode.h"

Sparse ODESolver::iterateRungeKutta4( const Sparse &rho, System &s, const double t, const double t_step, std::vector<SaveState> &savedStates ) { //FIXME: ts tep übergeben lol
    // Verschiedene H's fuer k1-4 ausrechnen
    Sparse H_calc_k1 = getHamilton( s, t );
    Sparse H_calc_k23 = getHamilton( s, t + t_step * 0.5 );
    Sparse H_calc_k4 = getHamilton( s, t + t_step );
    // k1-4 ausrechnen
    Sparse rk1 = s.dgl_rungeFunction( rho, H_calc_k1, t, savedStates );
    Sparse rk2 = s.dgl_rungeFunction( rho + t_step * 0.5 * rk1, H_calc_k23, t + t_step * 0.5, savedStates );
    Sparse rk3 = s.dgl_rungeFunction( rho + t_step * 0.5 * rk2, H_calc_k23, t + t_step * 0.5, savedStates );
    Sparse rk4 = s.dgl_rungeFunction( rho + t_step * rk3, H_calc_k4, t + t_step, savedStates );
    // Dichtematrix
    return rho + t_step / 6.0 * ( rk1 + 2. * rk2 + 2. * rk3 + rk4 );
}

Sparse ODESolver::iterateRungeKutta5( const Sparse &rho, System &s, const double t, const double t_step, std::vector<SaveState> &savedStates ) {
    // Verschiedene H's fuer k1-6 ausrechnen
    Sparse H_calc_k1 = getHamilton( s, t );
    Sparse H_calc_k2 = getHamilton( s, t + a2 * t_step );
    Sparse H_calc_k3 = getHamilton( s, t + a3 * t_step );
    Sparse H_calc_k4 = getHamilton( s, t + a4 * t_step );
    Sparse H_calc_k5 = getHamilton( s, t + a5 * t_step );
    Sparse H_calc_k6 = getHamilton( s, t + a6 * t_step );
    // k1-6 ausrechnen
    Sparse k1 = s.dgl_rungeFunction( rho, H_calc_k1, t, savedStates );
    Sparse k2 = s.dgl_rungeFunction( rho + t_step * b11 * k1, H_calc_k2, t + a2 * t_step, savedStates );
    Sparse k3 = s.dgl_rungeFunction( rho + t_step * ( b21 * k1 + b22 * k2 ), H_calc_k3, t + a3 * t_step, savedStates );
    Sparse k4 = s.dgl_rungeFunction( rho + t_step * ( b31 * k1 + b32 * k2 + b33 * k3 ), H_calc_k4, t + a4 * t_step, savedStates );
    Sparse k5 = s.dgl_rungeFunction( rho + t_step * ( b41 * k1 + b42 * k2 + b43 * k3 + b44 * k4 ), H_calc_k5, t + a5 * t_step, savedStates );
    Sparse k6 = s.dgl_rungeFunction( rho + t_step * ( b51 * k1 + b52 * k2 + b53 * k3 + b54 * k4 + b55 * k5 ), H_calc_k6, t + a6 * t_step, savedStates );
    // Dichtematrix
    return rho + t_step * ( b61 * k1 + b63 * k3 + b64 * k4 + b65 * k5 + b66 * k6 );
}

Sparse ODESolver::iterate( const Sparse &rho, System &s, const double t, const double t_step, std::vector<SaveState> &savedStates, const int dir ) {
    int order = dir == DIR_T ? s.parameters.numerics_order_t : s.parameters.numerics_order_tau;
    if ( order == 4 ) {
        return iterateRungeKutta4( rho, s, t, t_step, savedStates );
    }
    return iterateRungeKutta5( rho, s, t, t_step, savedStates );
}

bool ODESolver::calculate_runge_kutta( Sparse &rho0, double t_start, double t_end, double t_step_initial, Timer &rkTimer, ProgressBar &progressbar, std::string progressbar_name, System &s, std::vector<SaveState> &output, bool do_output ) {
    Log::L3( "Setting up Runge-Kutta Solver...\n" );
    // Reserve Output Vector
    output.reserve( s.parameters.iterations_t_max + 1 );
    // Save initial value
    saveState( rho0, t_start, output );
    // Calculate Remaining
    Sparse rho = rho0;
    Log::L3( "Calculating Runge-Kutta Loop...\n" );
    for ( double t_t = t_start + t_step_initial; t_t <= t_end; t_t += t_step_initial ) {
        // Runge-Kutta iteration
        rho = iterate( rho, s, t_t, t_step_initial, output );
        // Save Rho
        saveState( rho, t_t, output );
        // Progress and time output
        rkTimer.iterate();
        if ( do_output ) {
            Timers::outputProgress( s.parameters.output_handlerstrings, rkTimer, progressbar, s.parameters.iterations_t_max, progressbar_name );
        }
    }
    Log::L3( "Done!\n" );
    return true;
}