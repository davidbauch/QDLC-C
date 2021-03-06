#include "system/system.h"

// Phonon Contributions calculated via the Polaron Frame Approach
Scalar System::dgl_phonons_phi( const double t ) {
    Scalar integral = 0;
    double stepsize = 0.01 * parameters.p_phonon_wcutoff;
    double eV7 = convertParam<double>( "7.0eV" );
    double eV35 = -convertParam<double>( "3.5eV" );
    double v_c = 5110.0;
    double a_e = 5E-9;       //3E-9;
    double a_h = 0.87 * a_e; //a_e / 1.15;
    double rho = 5370.0;
    for ( double w = stepsize; w < 10 * parameters.p_phonon_wcutoff; w += stepsize ) {
        double J = parameters.p_phonon_alpha * w * std::exp( -w * w / 2.0 / parameters.p_phonon_wcutoff / parameters.p_phonon_wcutoff );
        //double J = w * parameters.hbar * std::pow( eV7 * std::exp( -w * w * a_e * a_e / ( 4. * v_c * v_c ) ) - eV35 * std::exp( -w * w * a_h * a_h / ( 4. * v_c * v_c ) ), 2. ) / ( 4. * 3.1415 * 3.1415 * rho * std::pow( v_c, 5. ) );
        integral += stepsize * ( J * ( std::cos( w * t ) / std::tanh( parameters.hbar * w / 2.0 / parameters.kb / parameters.p_phonon_T ) - 1i * std::sin( w * t ) ) );
    }
    return integral;
}

void System::initialize_polaron_frame_functions() {
    if ( parameters.p_phonon_T >= 0 ) {
        // Initialize Phi(tau)
        phi_vector.reserve( std::ceil( parameters.p_phonon_tcutoff / parameters.t_step * 3.1 ) );
        phi_vector.emplace_back( dgl_phonons_phi( parameters.t_step * 0.01 ) );
        for ( double tau = parameters.t_step; tau < parameters.p_phonon_tcutoff * 3.1; tau += parameters.t_step ) {
            phi_vector.emplace_back( dgl_phonons_phi( tau ) );
        }

        // Output Phonon Functions
        FILE *fp_phonons = std::fopen( ( parameters.subfolder + "phonons.txt" ).c_str(), "w" );
        fmt::print( fp_phonons, "t\treal(phi(t))\timag(phi(t))\treal(g_u(t))\timag(g_u(t))\treal(g_g(t))\timag(g_g(t))\n" );
        for ( double t = parameters.t_start; t < 3.0 * parameters.p_phonon_tcutoff; t += parameters.t_step ) {
            auto greenu = dgl_phonons_greenf( t, 'u' );
            auto greeng = dgl_phonons_greenf( t, 'g' );
            fmt::print( fp_phonons, "{}\t{}\t{}\t{}\t{}\t{}\t{}\n", t, std::real( phi_vector.at( std::floor( t / parameters.t_step ) ) ), std::imag( phi_vector.at( std::floor( t / parameters.t_step ) ) ), std::real( greenu ), std::imag( greenu ), std::real( greeng ), std::imag( greeng ) );
        }
        std::fclose( fp_phonons );
        if ( parameters.output_coefficients ) {
            fp_phonons = std::fopen( ( parameters.subfolder + "phonons_lb.txt" ).c_str(), "w" );
            fmt::print( fp_phonons, "t\tL_a_+\tL_a_-\tL_c_+\tL_c_-\n" );
            for ( double t = parameters.t_start; t < parameters.t_end; t += parameters.t_step ) {
                fmt::print( fp_phonons, "{}\t{}\t{}\t{}\t{}\n", t, dgl_phonons_lindblad_coefficients( t, 'L', 1.0 ), dgl_phonons_lindblad_coefficients( t, 'L', -1.0 ), dgl_phonons_lindblad_coefficients( t, 'C', 1.0 ), dgl_phonons_lindblad_coefficients( t, 'C', -1.0 ) );
            }
            std::fclose( fp_phonons );
        }
    }
}

Sparse System::dgl_phonons_rungefunc( const Sparse &chi, const double t ) {
    double chirpcorrection = chirp.get( t ) + t * ( chirp.get( t ) - parameters.scaleVariable( chirp.derivative( t ), parameters.scale_value ) );
    // FIX: für w_b-w_xi muss da -chirpcorrection statt +chirpcorrection!
    //FIXME: E_B(tau)!! -> 2de3lta - delta -> +delta statt -delta für B-X
    Sparse explicit_time = 1i * ( parameters.p_omega_atomic_G_H + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_G_H + 1i * ( parameters.p_omega_atomic_H_B + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_H_B + 1i * ( ( parameters.p_omega_atomic_H_B + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_H_B + ( parameters.p_omega_atomic_G_H + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_G_H - parameters.p_omega_cavity_H * operatorMatrices.projector_atom_sigmaplus_G_H - parameters.p_omega_cavity_H * operatorMatrices.projector_atom_sigmaplus_H_B ) * operatorMatrices.projector_photon_annihilate_H + parameters.scaleVariable( pulse_H.derivative( t ), parameters.scale_value ) * ( operatorMatrices.projector_atom_sigmaplus_G_H + operatorMatrices.projector_atom_sigmaplus_H_B );
    explicit_time += 1i * ( parameters.p_omega_atomic_G_V + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_G_V + 1i * ( parameters.p_omega_atomic_V_B + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_V_B + 1i * ( ( parameters.p_omega_atomic_V_B + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_V_B + ( parameters.p_omega_atomic_G_V + chirpcorrection ) * operatorMatrices.projector_atom_sigmaplus_G_V - parameters.p_omega_cavity_V * operatorMatrices.projector_atom_sigmaplus_G_V - parameters.p_omega_cavity_V * operatorMatrices.projector_atom_sigmaplus_V_B ) * operatorMatrices.projector_photon_annihilate_V + parameters.scaleVariable( pulse_V.derivative( t ), parameters.scale_value ) * ( operatorMatrices.projector_atom_sigmaplus_G_V + operatorMatrices.projector_atom_sigmaplus_V_B );
    explicit_time = parameters.scaleVariable( explicit_time, 1.0 / parameters.scale_value );

    Sparse hamilton = dgl_getHamilton( t );

    return -1i * dgl_kommutator( hamilton, chi ) + explicit_time.cwiseProduct( project_matrix_sparse( chi ) );
}

Sparse System::dgl_phonons_chiToX( const Sparse &chi, const char mode ) {
    Sparse adjoint = chi.adjoint();
    if ( mode == 'g' ) {
        return chi + adjoint;
    }
    return 1i * ( chi - adjoint );
}

Scalar System::dgl_phonons_greenf( double t, const char mode ) {
    int i = std::floor( t / parameters.t_step );
    auto phi = phi_vector.at( i ); //dgl_phonons_phi( t );
    if ( mode == 'g' ) {
        return parameters.p_phonon_b * parameters.p_phonon_b * ( std::cosh( phi ) - 1.0 );
    }
    return parameters.p_phonon_b * parameters.p_phonon_b * std::sinh( phi );
}

double System::dgl_phonons_lindblad_coefficients( double t, double omega_atomic, const char mode, const char level, const double sign ) {
    if ( t == 0.0 )
        t = parameters.t_step * 0.01;
    double ret = 0;
    double step = parameters.t_step;
    if ( mode == 'L' ) {
        double bpulsesquared, delta, nu;
        if ( level == 'H' ) {
            bpulsesquared = std::pow( std::abs( parameters.p_phonon_b * pulse_H.get( t ) ), 2.0 ); //TODO: chirp correction for omega_atomic!!!!
            delta = parameters.pulse_omega.at( 0 ) - omega_atomic;                                 //FIXME : different pulse frequencies
        } else {
            bpulsesquared = std::pow( std::abs( parameters.p_phonon_b * pulse_V.get( t ) ), 2.0 );
            delta = parameters.pulse_omega.at( 0 ) - omega_atomic; //FIXME : different pulse frequencies
        }
        nu = std::sqrt( bpulsesquared + delta * delta );
        int i = 0;
        for ( double tau = 0; tau < parameters.p_phonon_tcutoff; tau += step ) {
            Scalar f = ( delta * delta * std::cos( nu * tau ) + bpulsesquared ) / std::pow( nu, 2.0 );
            ret += std::real( ( std::cosh( phi_vector.at( i ) ) - 1.0 ) * f + std::sinh( phi_vector.at( i ) ) * std::cos( nu * tau ) ) - sign * std::imag( ( std::exp( phi_vector.at( i ) ) - 1.0 ) * delta * std::sin( nu * tau ) / nu );
            i++;
        }
        ret *= 2.0 * bpulsesquared * step;
    } else if ( mode == 'C' ) {
        double delta;
        if ( mode == 'H' ) {
            delta = parameters.p_omega_cavity_H - omega_atomic;
        } else {
            delta = parameters.p_omega_cavity_V - omega_atomic;
        }
        int i = 0;
        for ( double tau = 0; tau < parameters.p_phonon_tcutoff; tau += step ) {
            ret += std::real( std::exp( 1i * sign * delta * tau ) * ( std::exp( phi_vector.at( i ) ) - 1.0 ) );
            i++;
        }
        ret *= parameters.p_phonon_b * parameters.p_phonon_b * parameters.p_omega_coupling * parameters.p_omega_coupling * step;
    }
    return ret;
}

int System::dgl_get_coefficient_index( const double t, const double tau ) {
    if ( parameters.numerics_use_saved_coefficients ) {
        // Look for already calculated coefficient. if not found, calculate and save new coefficient
        if ( savedCoefficients.size() > 0 && t <= savedCoefficients.back().t && tau <= savedCoefficients.back().tau ) {
            track_getcoefficient_calcattempt++;
            double mult = ( parameters.numerics_phonon_approximation_markov1 ? std::floor( parameters.p_phonon_tcutoff / parameters.t_step ) : 1.0 );
            int add = ( parameters.numerics_phonon_approximation_markov1 ? (int)std::floor( ( parameters.p_phonon_tcutoff - tau ) / parameters.t_step ) : 0 );
            int approx = std::max( 0, std::min( (int)savedCoefficients.size() - 1, (int)( ( std::floor( t / parameters.t_step * 2.0 ) - 1.0 ) * mult ) ) - add ); //TODO: *4.0 wenn RK5, *parameters.rungekutta_timestep_multiplier
            int tries = 0;
            while ( approx < (int)savedCoefficients.size() - 1 && t > savedCoefficients.at( approx ).t ) {
                approx++;
                tries++;
            }
            while ( approx > 0 && t < savedCoefficients.at( approx ).t ) {
                approx--;
                tries++;
            }
            while ( approx < (int)savedCoefficients.size() - 1 && tau > savedCoefficients.at( approx ).tau ) {
                approx++;
                tries++;
            }
            while ( approx > 0 && tau < savedCoefficients.at( approx ).tau ) {
                approx--;
                tries++;
            }
            //Log::L2("Approx = {}, size = {}\n",approx,savedCoefficients.size());
            if ( approx < (int)savedCoefficients.size() ) {
                if ( t != savedCoefficients.at( approx ).t || tau != savedCoefficients.at( approx ).tau ) {
                    // This situation may occur during multithreading.
                    track_getcoefficient_read_but_unequal++;
                    //Log::L2( "Coefficient time mismatch! t = {} but coefficient.t = {}, tau = {} but coefficient.tau = {}\n", t, savedCoefficients.at( approx ).t, tau, savedCoefficients.at( approx ).tau );
                } else {
                    track_getcoefficient_read++;
                    globaltries += tries;
                    //Log::L2( "Coefficient time match! t = {} but coefficient.t = {}, tau = {} but coefficient.tau = {}\n", t, savedCoefficients.at( approx ).t, tau, savedCoefficients.at( approx ).tau );
                    return approx;
                }
            }
        }
    }
    return -1;
}

void System::dgl_save_coefficient( const Sparse &coefficient1, const Sparse &coefficient2, const double t, const double tau ) {
    track_getcoefficient_calculate++;
    if ( parameters.numerics_use_saved_coefficients && savedCoefficients.size() < parameters.numerics_saved_coefficients_max_size ) {
        track_getcoefficient_write++;
#pragma omp critical
        savedCoefficients.emplace_back( SaveStateTau( coefficient1, coefficient2, t, tau ) );
        // If use cutoff and vector saved more than 5 timesteps worth of matrices, delete other matrices
        if ( parameters.numerics_saved_coefficients_cutoff > 0 && savedCoefficients.size() > parameters.numerics_saved_coefficients_cutoff ) {
#pragma omp critical
            savedCoefficients.erase( savedCoefficients.begin() );
        }
        //Log::L2("Saved coefficient for t = {}, tau = {}\n",t,tau);
    }
}

Sparse System::dgl_phonons_chi( const double t ) {
    return dgl_timetrafo( parameters.p_omega_coupling * operatorMatrices.atom_sigmaplus_G_H * operatorMatrices.photon_annihilate_H + parameters.p_omega_coupling * operatorMatrices.atom_sigmaplus_H_B * operatorMatrices.photon_annihilate_H + ( operatorMatrices.atom_sigmaplus_G_H + operatorMatrices.atom_sigmaplus_H_B ) * pulse_H.get( t ) + parameters.p_omega_coupling * operatorMatrices.atom_sigmaplus_G_V * operatorMatrices.photon_annihilate_V + parameters.p_omega_coupling * operatorMatrices.atom_sigmaplus_V_B * operatorMatrices.photon_annihilate_V + ( operatorMatrices.atom_sigmaplus_G_V + operatorMatrices.atom_sigmaplus_V_B ) * pulse_V.get( t ), t );
}

Sparse System::dgl_phonons_calculate_transformation( Sparse &chi_tau, double t, double tau ) {
    if ( parameters.numerics_phonon_approximation_order == PHONON_APPROXIMATION_BACKWARDS_INTEGRAL ) {
        return Solver::calculate_definite_integral( chi_tau, std::bind( &System::dgl_phonons_rungefunc, this, std::placeholders::_1, std::placeholders::_2 ), t, std::max( t - tau, 0.0 ), -parameters.t_step ).mat;
    } else if ( parameters.numerics_phonon_approximation_order == PHONON_APPROXIMATION_TRANSFORMATION_MATRIX ) {
        Sparse U = ( Dense( -1i * dgl_getHamilton( t ) * tau ).exp() ).sparseView();
        return ( U * chi_tau * U.adjoint() ).eval();
    } else if ( parameters.numerics_phonon_approximation_order == PHONON_APPROXIMATION_MIXED ) {
        double error = std::abs( pulse_H.get( t ) + pulse_V.get( t ) );
        if ( ( pulse_H.maximum > 0 && error > pulse_H.maximum * 0.1 ) || ( pulse_V.maximum > 0 && error > pulse_V.maximum * 0.1 ) || chirp.derivative( t ) != 0 ) {
            return Solver::calculate_definite_integral( chi_tau, std::bind( &System::dgl_phonons_rungefunc, this, std::placeholders::_1, std::placeholders::_2 ), t, std::max( t - tau, 0.0 ), -parameters.t_step ).mat;
        }
        return chi_tau;
    } else {
        return chi_tau;
    }
}

Sparse System::dgl_phonons_pmeq( const Sparse &rho, const double t, const std::vector<SaveState> &past_rhos ) {
    Sparse ret = Sparse( parameters.maxStates, parameters.maxStates );
    Sparse chi = dgl_phonons_chi( t );
    std::vector<Sparse> threadmap_1, threadmap_2;

    // Most precise approximation used. Caclulate polaron fram Chi by integrating backwards from t to t-tau.
    if ( parameters.numerics_phonon_approximation_order != PHONON_APPROXIMATION_LINDBLAD_FULL ) {
        Sparse XUT = dgl_phonons_chiToX( chi, 'u' );
        Sparse XGT = dgl_phonons_chiToX( chi, 'g' );
        int _taumax = (int)std::min( parameters.p_phonon_tcutoff / parameters.t_step, t / parameters.t_step );
        // Index vector for thread ordering
        std::vector<int> thread_index;
        thread_index.reserve( _taumax );
        int thread_increment = std::ceil( _taumax / parameters.numerics_maximum_threads );
        for ( int thread = 0; thread < parameters.numerics_maximum_threads; thread++ ) {
            for ( int cur = 0; cur < parameters.numerics_maximum_threads; cur++ ) {
                int vec_index = thread + cur * parameters.numerics_maximum_threads;
                if ( vec_index < _taumax ) {
                    thread_index.emplace_back( vec_index );
                } else {
                    break;
                }
            }
        }
        // debug
        //Log::L2("Vec index:\n");
        //for ( auto &a : thread_index) {
        //    Log::L2("{}, ",a);
        //}
        //Log::L2("\nVec index done!\n");
        // Use markov approximation
        if ( parameters.numerics_phonon_approximation_markov1 ) {
            // Temporary variables
            Sparse chi_tau_back_u, chi_tau_back_g, integrant;
            // Look if we already calculated coefficient sum for this specific t-value
            int index = dgl_get_coefficient_index( t, 0 );
            if ( index != -1 ) {
                // Index was found, chi(t-tau) sum used from saved vector
                chi_tau_back_u = savedCoefficients.at( index ).mat1;
                chi_tau_back_g = savedCoefficients.at( index ).mat2;
            } else {
                // Index was not found, (re)calculate chi(t-tau) sum
                // Initialize temporary matrices to zero for threads to write to
                init_sparsevector( threadmap_1, parameters.maxStates, parameters.numerics_phonons_maximum_threads );
                init_sparsevector( threadmap_2, parameters.maxStates, parameters.numerics_phonons_maximum_threads );
                // Calculate backwards integral and sum it into threadmaps. Threadmaps will later be summed into one coefficient matrix.
#pragma omp parallel for ordered schedule( dynamic ) shared( savedCoefficients ) num_threads( parameters.numerics_phonons_maximum_threads )
                for ( int cur_thread = 0; cur_thread < parameters.numerics_phonons_maximum_threads; cur_thread++ ) {
                    for ( int cur = 0; cur < _taumax; cur += parameters.numerics_phonons_maximum_threads ) {
                        int _tau = cur_thread + cur;
                        //for ( int _tau = 0; _tau < _taumax; _tau++ ) {
                        Sparse chi_tau_back_u, chi_tau_back_g, chi_tau, chi_tau_back;
                        double tau = ( 1.0 * _tau ) * parameters.t_step;
                        chi_tau = dgl_phonons_chi( t - tau );
                        chi_tau_back = dgl_phonons_calculate_transformation( chi_tau, t, tau );
                        chi_tau_back_u = dgl_phonons_chiToX( chi_tau_back, 'u' );
                        chi_tau_back_g = dgl_phonons_chiToX( chi_tau_back, 'g' );
                        auto thread = omp_get_thread_num();
                        threadmap_1.at( thread ) += dgl_phonons_greenf( tau, 'u' ) * chi_tau_back_u;
                        threadmap_2.at( thread ) += dgl_phonons_greenf( tau, 'g' ) * chi_tau_back_g;
                    }
                }
                // Sum all contributions from threadmaps into one coefficient
                chi_tau_back_u = std::accumulate( threadmap_1.begin(), threadmap_1.end(), Sparse( parameters.maxStates, parameters.maxStates ) );
                chi_tau_back_g = std::accumulate( threadmap_2.begin(), threadmap_2.end(), Sparse( parameters.maxStates, parameters.maxStates ) );
                // Save coefficients
                dgl_save_coefficient( chi_tau_back_u, chi_tau_back_g, t, 0 );
            }
            // Calculate phonon contributions from (saved/calculated) coefficients and rho(t)
            integrant = dgl_kommutator( XUT, ( chi_tau_back_u * rho ).eval() );
            integrant += dgl_kommutator( XGT, ( chi_tau_back_g * rho ).eval() );
            Sparse adjoint = integrant.adjoint();
            ret -= ( integrant + adjoint ) * parameters.t_step;
        } else {
            init_sparsevector( threadmap_1, parameters.maxStates, parameters.numerics_phonons_maximum_threads );
            // Dont use markov approximation; Full integral
#pragma omp parallel for ordered schedule( dynamic ) shared( savedCoefficients ) num_threads( parameters.numerics_phonons_maximum_threads )
            for ( int cur_thread = 0; cur_thread < parameters.numerics_phonons_maximum_threads; cur_thread++ ) {
                for ( int cur = 0; cur < _taumax; cur += parameters.numerics_phonons_maximum_threads ) {
                    int _tau = cur_thread + cur;
                    //for ( int _tau = 0; _tau < _taumax; _tau++ ) {
                    Sparse chi_tau_back_u, chi_tau_back_g, chi_tau, chi_tau_back, integrant;
                    int rho_index = std::max( 0, (int)past_rhos.size() - 1 - _tau );
                    double tau = ( 1.0 * _tau ) * parameters.t_step;
                    // Check if chi(t-tau) has to be recalculated, or can just be retaken from saved matrices, since chi(t-tau) is only dependant on time
                    int index = dgl_get_coefficient_index( t, tau );
                    if ( index != -1 ) {
                        // Index was found, chi(t-tau) used from saved vector
                        chi_tau_back_u = savedCoefficients.at( index ).mat1;
                        chi_tau_back_g = savedCoefficients.at( index ).mat2;
                    } else {
                        // Index not found, or no saving is used. Recalculate chi(t-tau).
                        chi_tau = dgl_phonons_chi( t - tau );
                        chi_tau_back = chi_tau_back = dgl_phonons_calculate_transformation( chi_tau, t, tau );
                        chi_tau_back_u = dgl_phonons_chiToX( chi_tau_back, 'u' );
                        chi_tau_back_g = dgl_phonons_chiToX( chi_tau_back, 'g' );
                        // If saving is used, save current chi(t-tau). If markov approximation is used, only save final contributions
                        dgl_save_coefficient( chi_tau_back_u, chi_tau_back_g, t, tau );
                    }
                    integrant = dgl_phonons_greenf( tau, 'u' ) * dgl_kommutator( XUT, ( chi_tau_back_u * past_rhos.at( rho_index ).mat ).eval() );
                    integrant += dgl_phonons_greenf( tau, 'g' ) * dgl_kommutator( XGT, ( chi_tau_back_g * past_rhos.at( rho_index ).mat ).eval() );
                    Sparse adjoint = integrant.adjoint();
                    auto thread = omp_get_thread_num();
                    threadmap_1.at( thread ) += ( integrant + adjoint ) * parameters.t_step;
                }
            }
            ret -= std::accumulate( threadmap_1.begin(), threadmap_1.end(), Sparse( parameters.maxStates, parameters.maxStates ) );
        }
    } else if ( parameters.numerics_phonon_approximation_order == PHONON_APPROXIMATION_LINDBLAD_FULL ) {
        // H
        double chirpcorrection = chirp.get( t );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_H + chirpcorrection, 'L', 'H', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_G_H, operatorMatrices.atom_sigmaplus_G_H );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_H + chirpcorrection, 'L', 'H', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_G_H, operatorMatrices.atom_sigmaminus_G_H );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_H + chirpcorrection, 'C', 'H', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_G_H * operatorMatrices.photon_create_H, operatorMatrices.atom_sigmaplus_G_H * operatorMatrices.photon_annihilate_H );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_H + chirpcorrection, 'C', 'H', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_G_H * operatorMatrices.photon_annihilate_H, operatorMatrices.atom_sigmaminus_G_H * operatorMatrices.photon_create_H );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_H_B + chirpcorrection, 'L', 'H', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_H_B, operatorMatrices.atom_sigmaplus_H_B );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_H_B + chirpcorrection, 'L', 'H', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_H_B, operatorMatrices.atom_sigmaminus_H_B );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_H_B + chirpcorrection, 'C', 'H', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_H_B * operatorMatrices.photon_create_H, operatorMatrices.atom_sigmaplus_H_B * operatorMatrices.photon_annihilate_H );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_H_B + chirpcorrection, 'C', 'H', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_H_B * operatorMatrices.photon_annihilate_H, operatorMatrices.atom_sigmaminus_H_B * operatorMatrices.photon_create_H );
        // V
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_V + chirpcorrection, 'L', 'V', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_G_V, operatorMatrices.atom_sigmaplus_G_V );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_V + chirpcorrection, 'L', 'V', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_G_V, operatorMatrices.atom_sigmaminus_G_V );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_V + chirpcorrection, 'C', 'V', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_G_V * operatorMatrices.photon_create_V, operatorMatrices.atom_sigmaplus_G_V * operatorMatrices.photon_annihilate_V );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_G_V + chirpcorrection, 'C', 'V', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_G_V * operatorMatrices.photon_annihilate_V, operatorMatrices.atom_sigmaminus_G_V * operatorMatrices.photon_create_V );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_V_B + chirpcorrection, 'L', 'V', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_V_B, operatorMatrices.atom_sigmaplus_V_B );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_V_B + chirpcorrection, 'L', 'V', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_V_B, operatorMatrices.atom_sigmaminus_V_B );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_V_B + chirpcorrection, 'C', 'V', -1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaminus_V_B * operatorMatrices.photon_create_V, operatorMatrices.atom_sigmaplus_V_B * operatorMatrices.photon_annihilate_V );
        ret += dgl_phonons_lindblad_coefficients( t, parameters.p_omega_atomic_V_B + chirpcorrection, 'C', 'V', 1.0 ) * dgl_lindblad( rho, operatorMatrices.atom_sigmaplus_V_B * operatorMatrices.photon_annihilate_V, operatorMatrices.atom_sigmaminus_V_B * operatorMatrices.photon_create_V );
    }
    return ret;
}