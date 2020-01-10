//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) Sarah Jenkins and Richard F L Evans 2016. All rights reserved.
//
//   Email: sj681@york.ac.uk
//
//------------------------------------------------------------------------------
//

// Vampire headers
#include "micromagnetic.hpp"
#include "cells.hpp"
#include "internal.hpp"
#include "../cells/internal.hpp"
#include "../simulate/internal.hpp"
// micromagnetic module headers
#include <stdlib.h>
#include <vector>
#include "errors.hpp"
#include "sim.hpp"
#include "cells.hpp"
#include "vio.hpp"
#include "dipole.hpp"
#include "spintorque.hpp"
#include "environment.hpp"


namespace micromagnetic{
namespace internal{

   std::vector<double> calculate_llb_fields(std::vector <double > m,
                                            double temperature,
                                            int num_cells,
                                            int cell,
                                            std::vector<double>& x_array,
                                            std::vector<double>& y_array,
                                            std::vector<double>& z_array){

      //array to save the field to for each cell
      std::vector<double> spin_field(3,0.0);

      //chi is usually used as 1/2*chi
      const double one_o_2_chi_para = (one_o_chi_para[cell]*0.5);


      //the temperature is usually used as a reduced temperature or Tc/(Tc-T).
      const double reduced_temperature = temperature/Tc[cell];
      const double Tc_o_Tc_m_T = Tc[cell]/(temperature - Tc[cell]);


      //sets m_e and alpha temperature dependant parameters
      if (temperature<=Tc[cell]){
         m_e[cell] = pow((Tc[cell]-temperature)/(Tc[cell]),0.365);
         alpha_para[cell] = (2.0/3.0)*alpha[cell]*reduced_temperature;
         alpha_perp[cell] = alpha[cell]*(1.0-temperature/(3.0*Tc[cell]));
      }
      else{
         m_e[cell] = 0.0;
         alpha_para[cell] = alpha[cell]*(2.0/3.0)*reduced_temperature;
         alpha_perp[cell] = alpha_para[cell];
      }
      if (temperature < 0.1){
        m_e[cell] = 1.0;
        alpha_para[cell] = 0.001;
      }


      //m and me are usually used squared
      const double m_e_squared = m_e[cell]*m_e[cell];

      const double m_squared = m[1]*m[1]+m[2]*m[2]+m[0]*m[0];

      //calculates the intercell exchange field (pf) - this is dependent on temperature.
      double pf = 0;
      if(temperature<=Tc[cell]) pf = one_o_2_chi_para*(1.0 - m_squared/m_e_squared);
      else pf = -2.0*one_o_2_chi_para*(1.0 + Tc_o_Tc_m_T*3.0*m_squared/5.0);
      //std::cout << pf << '\t' << one_o_2_chi_para << '\t'  << m_squared/m_e_squared <<std::endl;
      //calculates the exchage fields as me^1.66 *A*(xi-xj)/m_e^2
      //array to store the exchanege field
      //std::cin.get();
      double exchange_field[3]={0.0,0.0,0.0};
      int mat  = cell_material_array[cell];
      if (num_cells > 1){
         //loops over all other cells with interactions to this cell
         const int start = macro_neighbour_list_start_index[cell];
         const int end = macro_neighbour_list_end_index[cell] +1;

         for(int j = start;j< end;j++){
            // calculate reduced exchange constant factor
            const int cellj = macro_neighbour_list_array[j];
            const double mj = sqrt(x_array[cellj]*x_array[cellj] + y_array[cellj]*y_array[cellj] + z_array[cellj]*z_array[cellj]);
            double Ac = A[j]*pow(mj,1.66);

            int matj =cell_material_array[cellj];
            if (cell != cellj){
            if (mp::material[mat].enable_SAF == true && mp::material[matj].enable_SAF == true){
              if (mat != matj){
            //    std::cout << cell << '\t' << cellj << "\t" << Ac << std::endl;
              Ac = -pow(mj,1.66)*prefactor[matj]*mp::material[mat].SAF[matj];
              //  std::cout << mat << '\t' << matj << "\t" << Ac << std::endl;

              }
           }

           if (mp::material[mat].override_atomsitic[matj] == true){
             //double Area = cells::macro_cell_size_x*cells::macro_cell_size_y;
             //double Volume = cells::macro_cell_size_x*cells::macro_cell_size_y*cells::macro_cell_size_z;
             //double Ac2 = Ac;
             Ac = -2*pow(mj,1.66)*mp::material[mat].EF_MM[matj]/(ms[cell]);
           }
        //   std::cout << cell << '\t' << cellj << '\t' << -Ac << '\t' <<  std::endl;
            exchange_field[0] -= Ac*(x_array[cellj] - x_array[cell]);
            exchange_field[1] -= Ac*(y_array[cellj] - y_array[cell]);
            exchange_field[2] -= Ac*(z_array[cellj] - z_array[cell]);
          }
         }
      }

      //exchange_field[0] = 0.0;
      //exchange_field[1] = 0.0;
      //exchange_field[2] = 0.0;

      const double stpx = sim::internal::slonczewski_spin_polarization_unit_vector[0];
  		const double stpy = sim::internal::slonczewski_spin_polarization_unit_vector[1];
  		const double stpz = sim::internal::slonczewski_spin_polarization_unit_vector[2];

  		const double staj = sim::internal::slonczewski_aj[mat];
  		const double stbj = sim::internal::slonczewski_bj[mat];

  		// calculate field
  		double HSTTx = staj*(y_array[cell]*stpz - z_array[cell]*stpy) + stbj*stpx;
  		double HSTTy = staj*(z_array[cell]*stpx - x_array[cell]*stpz) + stbj*stpy;
  		double HSTTz = staj*(x_array[cell]*stpy - y_array[cell]*stpx) + stbj*stpz;

      // std::cout << HSTTx << '\t' << HSTTy << '\t' << HSTTz << std::endl;

      //Sum H = H_exch + H_A +H_exch_grains +H_App + H+dip
    //  std::cout << cell << '\t' << pinning_field_x[cell] << '\t' << pinning_field_y[cell] << '\t' << pinning_field_z[cell] << std::endl;
    //  std::cout << cell_material_array[cell]<<'\t' << pinning_field_x[cell]  + exchange_field[0] << '\t' << pinning_field_y[cell]  + exchange_field[1]<< "\t" << pinning_field_z[cell]  + exchange_field[2] << std::endl;
      spin_field[0] = pf*m[0] + ext_field[0]  + pinning_field_x[cell]  + exchange_field[0] - ku_x[cell]*one_o_chi_perp[cell]*m[0] + HSTTx;// + dipole::cells_field_array_x[cell];
      spin_field[1] = pf*m[1] + ext_field[1]  + pinning_field_y[cell]  + exchange_field[1] - ku_y[cell]*one_o_chi_perp[cell]*m[1] + HSTTy;// + dipole::cells_field_array_y[cell];
      spin_field[2] = pf*m[2] + ext_field[2]  + pinning_field_z[cell]  + exchange_field[2] - ku_z[cell]*one_o_chi_perp[cell]*m[2] + HSTTz;// + dipole::cells_field_array_z[cell];
    //  std::cout << ku_x[cell]*one_o_chi_perp[cell] << '\t' << ku_y[cell]*one_o_chi_perp[cell] << '\t' << ku_z[cell]*one_o_chi_perp[cell] << std::endl;
        //if (cell ==0)		std::cout  << "inside\t" <<   dipole::cells_field_array_x[0] << '\t' << dipole::cells_field_array_y[0] << '\t' << dipole::cells_field_array_z[0] <<std::endl;
 //       std::cin.get();
  //}
  if (dipole::activated){
 // std::cout << environment::environment_field_x[cell] << '\t' << environment::environment_field_x[cell]  << '\t' << environment::environment_field_x[cell] << std::endl;
    spin_field[0] = spin_field[0] + dipole::cells_field_array_x[cell];
    spin_field[1] = spin_field[1] + dipole::cells_field_array_y[cell];
    spin_field[2] = spin_field[2] + dipole::cells_field_array_z[cell];
 }
 //std::cout << "SP" << spin_field[0] << "\t" << spin_field[1] << '\t' << spin_field[2] << std::endl;
    if (environment::enabled){
  // std::cout << environment::environment_field_x[cell] << '\t' << environment::environment_field_y[cell]  << '\t' << environment::environment_field_z[cell] << std::endl;
       spin_field[0] = spin_field[0] + environment::environment_field_x[cell];
       spin_field[1] = spin_field[1] + environment::environment_field_y[cell];
       spin_field[2] = spin_field[2] + environment::environment_field_z[cell];
    }

     if (sim::track_field_x.size() != 0 ){
       spin_field[0] = spin_field[0] + sim::track_field_x[cell];
       spin_field[1] = spin_field[1] + sim::track_field_y[cell];
       spin_field[2] = spin_field[2] + sim::track_field_z[cell];
    //   std::cout << pinning_field_y[cell] << '\t' << sim::track_field_x[cell] << '\t' << sim::track_field_y[cell] << "\t" <<sim::track_field_z[cell] <<std::endl;
     }

     if (bias_magnets == true){
    //    std::cout << spin_field[0] << '\t' << spin_field[1] << '\t' << spin_field[2]<< "\t" << bias_field_x[cell] << '\t' << bias_field_y[cell] << '\t' << bias_field_z[cell] <<std::endl;
       spin_field[0] = spin_field[0] + bias_field_x[cell];
       spin_field[1] = spin_field[1] + bias_field_y[cell];
       spin_field[2] = spin_field[2] + bias_field_z[cell];
     }

      //std::cout << environment::environment_field_x[cell] << '\t' << environment::environment_field_x[cell]  << '\t' << environment::environment_field_x[cell] << std::endl;
      return spin_field;
   }

} // end of internal namespace

} // end of micromagnetic namespace
