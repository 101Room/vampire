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

// C++ standard library headers

// Vampire headers
#include "micromagnetic.hpp"

// micromagnetic module headers
#include "internal.hpp"
#include "material.hpp"
#include "cells.hpp"
#include "../cells/internal.hpp"
#include "../create/internal.hpp"
#include "atoms.hpp"
#include "vmpi.hpp"


#include <iostream>

namespace mm = micromagnetic::internal;
using namespace std;

namespace micromagnetic{

   //----------------------------------------------------------------------------
   // Function to initialize micromagnetic module
   //----------------------------------------------------------------------------
   void initialize(int num_local_cells,
                   int num_cells,
                   int num_atoms,
                   int num_materials,
                   std::vector<int> cell_array,                     //1D array storing which cell each atom is in
                   std::vector<int> neighbour_list_array,           //1D vector listing the nearest neighbours of each atom
                   std::vector<int> neighbour_list_start_index,     //1D vector storing the start index for each atom in the neighbour_list_array
                   std::vector<int> neighbour_list_end_index,       //1D vector storing the end index for each atom in the neighbour_list_array
                   const std::vector<int> type_array,               //1D array storing which material each cell is
                   std::vector <mp::materials_t> material,          //1D vector of type material_t stiring the material properties
                   std::vector <double> x_coord_array,
                   std::vector <double> y_coord_array,
                   std::vector <double> z_coord_array,
                   std::vector <double> volume_array,               //1D vector storing the volume of each cell
                   double Temperature,
                   double num_atoms_in_unit_cell,
                   double system_dimensions_x,
                   double system_dimensions_y,
                   double system_dimensions_z,
                   std::vector<int> local_cell_array){

   // Output informative message to user
   std::cout << "Initialising micromagnetic module" << std::endl;

   int num_atoms_interactions;
   #ifdef MPICF
      num_atoms_interactions = vmpi::num_core_atoms+vmpi::num_bdry_atoms + vmpi::num_halo_atoms;
      num_atoms = vmpi::num_core_atoms+vmpi::num_bdry_atoms;
   #else
      num_atoms_interactions = num_atoms;
   #endif


   mm::A.resize(num_cells*num_cells,0.0);
   mm::alpha.resize(num_cells,0.0);
   mm::one_o_chi_perp.resize(num_cells,0.0);
   mm::one_o_chi_para.resize(num_cells,0.0);
   mm::gamma.resize(num_cells,0.0);
   mm::ku.resize(num_cells,0.0);
   mm::ku_x.resize(num_cells,0.0);
   mm::ku_y.resize(num_cells,0.0);
   mm::ku_z.resize(num_cells,0.0);
   mm::ms.resize(num_cells,0.0);
   mm::T.resize(num_cells,0.0);
   mm::Tc.resize(num_cells,0.0);
   mm::alpha_para.resize(num_cells,0.0);
   mm::alpha_perp.resize(num_cells,0.0);
   mm::m_e.resize(num_cells,0.0);
   mm::macro_neighbour_list_start_index.resize(num_cells,0.0);
   mm::macro_neighbour_list_end_index.resize(num_cells,0.0);
   micromagnetic::cell_discretisation_micromagnetic.resize(num_cells,true);
   mm::ext_field.resize(3,0.0);
   mm::pinning_field_x.resize(num_cells,0.0);
   mm::pinning_field_y.resize(num_cells,0.0);
   mm::pinning_field_z.resize(num_cells,0.0);
   mm::cell_material_array.resize(num_cells,0.0);
   mm::bias_field_x.resize(num_cells,0.0);
   mm::bias_field_y.resize(num_cells,0.0);
   mm::bias_field_z.resize(num_cells,0.0);

   // These functions vectors with the parameters calcualted from the function
   mm::ms =                   mm::calculate_ms(num_local_cells,num_atoms,num_cells, cell_array, type_array,material,local_cell_array);
   mm::alpha =                mm::calculate_alpha(num_local_cells,num_atoms, num_cells, cell_array, type_array, material,local_cell_array);
   mm::Tc =                   mm::calculate_tc(num_local_cells, local_cell_array,num_atoms_interactions, num_cells, cell_array,neighbour_list_array,
                                               neighbour_list_start_index, neighbour_list_end_index, type_array, material);
   mm::ku =                   mm::calculate_ku(num_atoms, num_cells, cell_array, type_array, material);
   mm::gamma =                mm::calculate_gamma(num_atoms, num_cells, cell_array,type_array,material,num_local_cells,local_cell_array);
   //mm::one_o_chi_para =       mm::calculate_chi_para(num_local_cells, local_cell_array,num_cells, Temperature);
   //mm::one_o_chi_perp =       mm::calculate_chi_perp(num_local_cells, local_cell_array,num_cells, Temperature);

   mm::A =                    mm::calculate_a(num_atoms_interactions, num_cells, num_local_cells,cell_array, neighbour_list_array, neighbour_list_start_index,
                                              neighbour_list_end_index, type_array,  material, volume_array, x_coord_array,
                                              y_coord_array, z_coord_array, num_atoms_in_unit_cell, local_cell_array);

   //std::cout << mm::ms[0] << '\t' << mm::alpha[0] << '\t' << mm::Tc[0] << '\t' << mm::ku[0] << '\t' << mm::gamma[0] << '\t' << mm::one_o_chi_para[0] << '\t' << mm::one_o_chi_perp[0] << '\t' << mm::A[0] << '\t' << std::endl;

    for (int cell = 0; cell < num_cells; cell++)
         if (discretisation_type == 1){
            for (int lc = 0; lc < num_local_cells; lc++){
               int cell = local_cell_array[lc];
               if (mm::Tc[cell] < 0) {
                  discretisation_type = 2;
               }
            }
         }

         //if multiscale simulation work out which cells/atoms are micromagnetic/atomistic
         if (discretisation_type == 2){
            //loops over all atoms and if any atom in the cell is atomistic the whole cell becomes atomistic else the cell is micromagnetic.
            for (int atom =0; atom < num_atoms; atom++){
               int cell = cell_array[atom];
               int mat  = type_array[atom];
               micromagnetic::cell_discretisation_micromagnetic[cell] = mp::material[mat].micromagnetic_enabled;
               //unless the cell contains AFM atoms, then it is always atomsitic
               if (mm::Tc[cell] < 0) micromagnetic::cell_discretisation_micromagnetic[cell] = 0;

            }

            //loops over all atoms saves each atom at micromagnetic or atomistic depending on whether the cell is microamgnetic or atomistic
            for (int atom =0; atom < num_atoms; atom++){
               int cell = cell_array[atom];
               //id atomistic add to numner of atomisic atoms
               if (micromagnetic::cell_discretisation_micromagnetic[cell] == 0) {
                  list_of_atomistic_atoms.push_back(atom);
                  number_of_atomistic_atoms++;
               }
               //if micromagnetic add to the numebr of microamgnetic cells.
               else {
                  list_of_none_atomistic_atoms.push_back(atom);
                  number_of_none_atomistic_atoms++;
               }
            }

            //if simualtion is micromagetic all cells are made micromagnetic cells
            for (int lc = 0; lc < num_local_cells; lc++){
               int cell = local_cell_array[lc];
               // need to fix here tOO!!!
               if (micromagnetic::cell_discretisation_micromagnetic[cell] == 1 && mm::ms[cell] > 1e-30) {
                  list_of_micromagnetic_cells.push_back(cell);
                  number_of_micromagnetic_cells ++;
               }
               // Otherwise list cells as non-magnetic (empty)
               else{
                  list_of_empty_micromagnetic_cells.push_back(cell);
               }
            }

         }

         //if micromagnetic simulation all cells are micromagnetic and all atoms are micromagnetic
         else {

            vmpi::barrier();

            std::vector<int> list_of_magnetic_cells(0);

            // determine total number of magnetic cells
            int num_magnetic_cells = 0;
            for(int c = 0; c < cells::num_cells; c++){
               if(mm::ms[c] > 1e-40 && mm::Tc[c] > 0.1){
                  list_of_magnetic_cells.push_back(c);
                  num_magnetic_cells++;
               }
               else{
                  list_of_empty_micromagnetic_cells.push_back(c);
               }
            }


            // now divide cells over processors, allocating extra to the last processor
            int num_cells_pp = num_magnetic_cells / vmpi::num_processors;
            int my_num_cells = num_cells_pp;
            if(vmpi::my_rank == vmpi::num_processors - 1) my_num_cells = num_magnetic_cells - (vmpi::num_processors-1)*num_cells_pp;

            std::cerr << vmpi::my_rank << "\t" << cells::num_cells << "\t" << my_num_cells << "\t" << num_magnetic_cells << std::endl;

            // loop over all magnetic cells, allocating cells to processors in linear fashion
            const int start = vmpi::my_rank*num_cells_pp;
            const int end   = vmpi::my_rank*num_cells_pp + my_num_cells;
            for(int c = start; c < end; c++){
               list_of_micromagnetic_cells.push_back( list_of_magnetic_cells[c] );
               number_of_micromagnetic_cells ++;
            }

            vmpi::barrier();

            /*for (int lc = 0; lc < num_local_cells; lc++){
               int cell = local_cell_array[lc];

               // Check that cell is micromagnetic, has a reasonable moment and reasonable Curie temperature
               if(mm::ms[cell] > 1e-40 && mm::Tc[cell] > 0.1){

                  // save cell in list
                  list_of_micromagnetic_cells.push_back(cell);

                  // update number of cells
                  number_of_micromagnetic_cells ++;

               }
               // Otherwise list cells as non-magnetic (empty)
               else{
                  list_of_empty_micromagnetic_cells.push_back(cell);
               }

            }*/

            for (int atom =0; atom < num_atoms; atom++){
               list_of_none_atomistic_atoms.push_back(atom);
               number_of_none_atomistic_atoms++;
            }

         }

         //for field calculations you need to access the atoms in numerically consecutive lists.
         //therefore you need to create lists of consecutive lists
         //loops over all atoms if atom is not one minus the previous atom then create a new list.

         if (number_of_atomistic_atoms > 0){
            int end = list_of_atomistic_atoms[0];
            int begin = list_of_atomistic_atoms[0];
            for(int atom_list=1;atom_list<number_of_atomistic_atoms;atom_list++){
               int atom = list_of_atomistic_atoms[atom_list];
               int last_atom = list_of_atomistic_atoms[atom_list - 1];
               if ((atom != last_atom +1) || (atom_list == number_of_atomistic_atoms -1)){
                  end = atom +1;
                  mm::fields_neighbouring_atoms_begin.push_back(begin);
                  mm::fields_neighbouring_atoms_end.push_back(end);

                  begin = atom + 1;
               }
            }
         }

         //--------------------------------------------------------------------------------------------------
         // Pre-calculate micromagnetic parameters
         //--------------------------------------------------------------------------------------------------
         //mm::calculate_chi_para(number_of_micromagnetic_cells, list_of_micromagnetic_cells, mm::one_o_chi_para, mm::T, mm::Tc);
         //mm::calculate_chi_perp(number_of_micromagnetic_cells, list_of_micromagnetic_cells, mm::one_o_chi_perp, mm::T, mm::Tc);


        /*for (int cell = 0; cell < num_cells; cell++){
          std::cerr << '\t' << cells::cell_coords_array_z[cell] << '\t' <<  mm::ms[cell] << '\t' << mm::ku[cell] << '\t' << mm::A[cell] << "\t" << mm::Tc[cell] << "\t" <<micromagnetic::cell_discretisation_micromagnetic[cell] << "\t" << mm::alpha[cell] << '\t' << mm::gamma[cell] << std::endl;
       }*/

        std::vector < double > temp(num_cells,0);


        //int num_calculations = mm::fields_neighbouring_atoms_begin.size();

        for (int atom = 0; atom < num_atoms_interactions; atom ++){
           int mat = type_array[atom];
           int cell = cell_array[atom];
           mm::cell_material_array[cell] = mat;
        }

      //  std::vector <int> n_cells_per_mat(mp::num_materials,0);
        mm::mat_vol.resize(mp::num_materials,0);
        mm::mat_ms.resize(mp::num_materials,0);
        mm::prefactor.resize(mp::num_materials,0);
        for (int cell = 0; cell < num_cells; cell++ ){
            int mat = mm::cell_material_array[cell];
          //  n_cells_per_mat[mat]++;
            mm::mat_vol[mat] = mm::mat_vol[mat] + volume_array[cell];
            mm::mat_ms[mat] = mm::mat_ms[mat] + mm::ms[cell];
        }
        for (int mat = 0; mat < mp::num_materials; mat++ ){
          double min=create::internal::mp[mat].min;
          double max=create::internal::mp[mat].max;
          double t = max - min;
          t = t*system_dimensions_z;
          mm::prefactor[mat] = 1/(t*mm::mat_ms[mat]/mm::mat_vol[mat]);
        //  std::cout << mm::mat_vol[mat] <<std::endl;
        }

        for (int cell = 0; cell < num_cells; cell++ ){

           //double zi = cells::pos_and_mom_array[4*cell+2];
            int mat = mm::cell_material_array[cell];

            if (mp::material[mat].pinning_field_unit_vector[0]+ mp::material[mat].pinning_field_unit_vector[1] + mp::material[mat].pinning_field_unit_vector[2]!= 0.0){
              //double Area = cells::macro_cell_size_x*cells::macro_cell_size_y;
            //  std::cout << mp::material[mat].pinning_field_unit_vector[0] << '\t' <<mp::material[mat].pinning_field_unit_vector[1] << '\t' << mp::material[mat].pinning_field_unit_vector[2] << '\t' << mm::ms[cell] << '\t' << Area << std::endl;
               mm::pinning_field_x[cell] = mm::prefactor[mat]*mp::material[mat].pinning_field_unit_vector[0];
               mm::pinning_field_y[cell] = mm::prefactor[mat]*mp::material[mat].pinning_field_unit_vector[1];
               mm::pinning_field_z[cell] = mm::prefactor[mat]*mp::material[mat].pinning_field_unit_vector[2];
            //   std::cout <<  mm::pinning_field_x[cell] << '\t' << mm::pinning_field_y[cell] << '\t' <<  mm::pinning_field_z[cell] << std::endl;

              // std::cout <<prefactor*mp::material[mat].pinning_field_unit_vector[2] << '\t' << prefactor2*mp::material[mat].pinning_field_unit_vector[2] << '\t' << prefactor3*mp::material[mat].pinning_field_unit_vector[2] << '\t' << prefactor4*mp::material[mat].pinning_field_unit_vector[2] <<  std::endl;
//               n_cells

           }
        }




       if (mm::mm_correction == true){
          for (int cell = 0; cell < num_cells; cell++ ){
             mm::pinning_field_x[cell] = 2*mm::pinning_field_x[cell]/cells::macro_cell_size_x;
             mm::pinning_field_y[cell] = 2*mm::pinning_field_y[cell]/cells::macro_cell_size_y;
             mm::pinning_field_z[cell] = 2*mm::pinning_field_z[cell]/cells::macro_cell_size_z;

          }
     }
      if (enable_resistance && mm::resistance_layer_2 != mm::resistance_layer_1 ){
        for (int cell = 0; cell < cells::num_cells; cell++ ){

          int mat =mm::cell_material_array[cell];
          const int start = mm::macro_neighbour_list_start_index[cell];
          const int end = mm::macro_neighbour_list_end_index[cell] +1;

          for(int j = start;j< end;j++){
             // calculate reduced exchange constant factor

             const int cellj = mm::macro_neighbour_list_array[j];
             int matj =mm::cell_material_array[cellj];

            if (mat == mm::resistance_layer_1 && matj == mm::resistance_layer_2){
        //      std::cout << mm::resistance_layer_1 << '\t' << mm::resistance_layer_2 <<
              mm::overlap_area = mm::overlap_area + cells::macro_cell_size_x*cells::macro_cell_size_y;

            }
          }
        }
      }

      //std::cout << "OVERLAP" << "\t" << mm::overlap_area << std::endl;
      mm::overlap_area = 240*200;

        // //boltzman stuff
        // P.resize(101);
        // for (int i = 0; i < 101; i++) P[i].resize(101,0.0);


      if(mm::bias_magnets == true){
         mm::calculate_bias_magnets(system_dimensions_x,system_dimensions_y,system_dimensions_z);
      }
      std::cout << "End of micromangetic initialisation" << std::endl;

   return;

   }

} // end of micromagnetic namespace
