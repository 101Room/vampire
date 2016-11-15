//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) Richard F L Evans 2016. All rights reserved.
//
//   Email: richard.evans@york.ac.uk
//
//------------------------------------------------------------------------------
//

// C++ standard library headers
#include <string>

// Vampire headers
#include "unitcell.hpp"
#include "errors.hpp"
#include "vio.hpp"

// unitcell module headers
#include "internal.hpp"

namespace unitcell{

   //---------------------------------------------------------------------------
   // Function to process input file parameters for unitcell module
   //---------------------------------------------------------------------------
   bool match_input_parameter(std::string const key, std::string const word, std::string const value, std::string const unit, int const line){

      // Check for first prefix
      std::string prefix="unit-cell";
      std::string test = "";

      // Check for second prefix
      prefix="material";
      if(key == prefix){
         //-------------------------------------------------------------------
   		// Get unit cell filename
   		//-------------------------------------------------------------------
         test="unit-cell-file";
   		if(word==test){
   			std::string ucffile=value;
   			// strip quotes
   			ucffile.erase(remove(ucffile.begin(), ucffile.end(), '\"'), ucffile.end());
   			test="";
   			// if filename not blank set ucf file name
   			if(ucffile!=test){
   				//std::cout << matfile << std::endl;
   				uc::internal::unit_cell_filename=ucffile;
   				return true;
   			}
   			else{
   				terminaltextcolor(RED);
   				std::cerr << "Error - empty filename in control statement \'material:" << word << "\' on line " << line << " of input file" << std::endl;
   				terminaltextcolor(WHITE);
   				return false;
   			}
   		}
      }

      // Check for third prefix
      prefix="create";
      if(key == prefix){
         //--------------------------------------------------------------------
         test="crystal-structure";
         if(word==test){
            // Strip quotes
            std::string cs=value;
            cs.erase(remove(cs.begin(), cs.end(), '\"'), cs.end());
            uc::internal::crystal_structure=cs;
            return true;
         }
      }
      // Check for final prefix
      prefix="dimensions";
      if(key == prefix){
         test="unit-cell-size";
         if(word==test){
            double a=atof(value.c_str());
            vin::check_for_valid_value(a, word, line, prefix, unit, "length", 0.1, 1.0e7,"input","0.1 Angstroms - 1 millimetre");
            uc::internal::unit_cell_size_x=a;
            uc::internal::unit_cell_size_y=a;
            uc::internal::unit_cell_size_z=a;
            return true;
         }
         //--------------------------------------------------------------------
         test="unit-cell-size-x";
         if(word==test){
            double ax=atof(value.c_str());
            vin::check_for_valid_value(ax, word, line, prefix, unit, "length", 0.1, 1.0e7,"input","0.1 Angstroms - 1 millimetre");
            uc::internal::unit_cell_size_x=ax;
            return true;
         }
         //--------------------------------------------------------------------
         test="unit-cell-size-y";
         if(word==test){
            double ay=atof(value.c_str());
            vin::check_for_valid_value(ay, word, line, prefix, unit, "length", 0.1, 1.0e7,"input","0.1 Angstroms - 1 millimetre");
            uc::internal::unit_cell_size_y=ay;
            return true;
         }
         //--------------------------------------------------------------------
         test="unit-cell-size-z";
         if(word==test){
            double az=atof(value.c_str());
            vin::check_for_valid_value(az, word, line, prefix, unit, "length", 0.1, 1.0e7,"input","0.1 Angstroms - 1 millimetre");
            uc::internal::unit_cell_size_z=az;
            return true;
         }
      }
      //--------------------------------------------------------------------
      // Keyword not found
      //--------------------------------------------------------------------
      return false;

   }

   //---------------------------------------------------------------------------
   // Function to process material parameters
   //---------------------------------------------------------------------------
   bool match_material_parameter(std::string const word, std::string const value, std::string const unit, int const line, int const super_index, const int sub_index){

      // add prefix string
      std::string prefix="material:";

      // exchange-matrix[i][j] = val // current
      // exchange-matrix[i][j] = xx, yy, zz // vectorial(new)
      // exchange-interaction-range = 1...6 (shells), 0.01-1000 Angstroms
      // exchange-function = nearest-neighbour, exponential, r4, rkky, file.dat (s, Jij)
      //

      //--------------------------------------------------------------------
      // Keyword not found
      //--------------------------------------------------------------------
      return false;

   }

} // end of unitcell namespace
