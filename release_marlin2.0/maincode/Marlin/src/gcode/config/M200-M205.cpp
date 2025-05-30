/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../gcode.h"
#include "../../MarlinCore.h"
#include "../../module/planner.h"

#if DISABLED(NO_VOLUMETRICS)

  /**
   * M200: Set filament diameter and set E axis units to cubic units
   *
   *    T<extruder> - Optional extruder number. Current extruder if omitted.
   *    D<linear>   - Set filament diameter and enable. D0 disables volumetric.
   *    S<bool>     - Turn volumetric ON or OFF.
   *    L<float>    - Volumetric extruder limit (in mm^3/sec). L0 disables the limit.
   */
  void GcodeSuite::M200() {

    const int8_t target_extruder = get_target_extruder_from_command();
    if (target_extruder < 0) return;

    bool vol_enable = parser.volumetric_enabled,
         can_enable = true;

    if (parser.seenval('D')) {
      const float dval = parser.value_linear_units();
      if (dval) { // Set filament size for volumetric calculation
        planner.set_filament_size(target_extruder, dval);
        vol_enable = true;    // Dn = enable for compatibility
      }
      else
        can_enable = false;   // D0 = disable for compatibility
    }

    // Enable or disable with S1 / S0
    parser.volumetric_enabled = can_enable && parser.boolval('S', vol_enable);

    #if ENABLED(VOLUMETRIC_EXTRUDER_LIMIT)
      if (parser.seenval('L')) {
        // Set volumetric limit (in mm^3/sec)
        const float lval = parser.value_float();
        if (WITHIN(lval, 0, 20))
          planner.set_volumetric_extruder_limit(target_extruder, lval);
        else
          SERIAL_ECHOLNPGM("?L value out of range (0-20).");
      }
    #endif

    planner.calculate_volumetric_multipliers();
  }

#endif // !NO_VOLUMETRICS

/**
 * M201: Set max acceleration in units/s^2 for print moves (M201 X1000 Y1000)
 *
 *       With multiple extruders use T to specify which one.
 */
void GcodeSuite::M201() {

  const int8_t target_extruder = get_target_extruder_from_command();
  if (target_extruder < 0) return;

  #ifdef XY_FREQUENCY_LIMIT
    if (parser.seenval('F')) planner.set_frequency_limit(parser.value_byte());
    if (parser.seenval('G')) planner.xy_freq_min_speed_factor = constrain(parser.value_float(), 1, 100) / 100;
  #endif

  LOOP_LOGICAL_AXES(i) {
    if (parser.seenval(axis_codes[i])) {
      const uint8_t a = TERN(HAS_EXTRUDERS, (i == E_AXIS ? uint8_t(E_AXIS_N(target_extruder)) : i), i);
      planner.set_max_acceleration(a, parser.value_axis_units((AxisEnum)a));
    }
  }
}

/**
 * M203: Set maximum feedrate that your machine can sustain (M203 X200 Y200 Z300 E10000) in units/sec
 *
 *       With multiple extruders use T to specify which one.
 */
void GcodeSuite::M203() {

  const int8_t target_extruder = get_target_extruder_from_command();
  if (target_extruder < 0) return;

  LOOP_LOGICAL_AXES(i)
    if (parser.seenval(axis_codes[i])) {
      const uint8_t a = TERN(HAS_EXTRUDERS, (i == E_AXIS ? uint8_t(E_AXIS_N(target_extruder)) : i), i);
      planner.set_max_feedrate(a, parser.value_axis_units((AxisEnum)a));
    }
}

/**
 * M204: Set Accelerations in units/sec^2 (M204 P1200 R3000 T3000)
 *
 *    P = Printing moves
 *    R = Retract only (no X, Y, Z) moves
 *    T = Travel (non printing) moves
 */
void GcodeSuite::M204() {
  const float default_accel   = (planner.LIN_ADV_version_change >= LIN_ADV_VERSION_2) ? LA_V1_DEFAULT_ACCELERATION         : DEFAULT_ACCELERATION;
  const float default_retract = (planner.LIN_ADV_version_change >= LIN_ADV_VERSION_2) ? LA_V1_DEFAULT_RETRACT_ACCELERATION : DEFAULT_RETRACT_ACCELERATION;
  const float default_travel  = (planner.LIN_ADV_version_change >= LIN_ADV_VERSION_2) ? LA_V1_DEFAULT_TRAVEL_ACCELERATION  : DEFAULT_TRAVEL_ACCELERATION;

  if (!parser.seen("PRST")) {
    SERIAL_ECHOPAIR("Acceleration: P", planner.settings.acceleration);
    SERIAL_ECHOPAIR(" R", planner.settings.retract_acceleration);
    SERIAL_ECHOLNPAIR_P(SP_T_STR, planner.settings.travel_acceleration);
  }
  else {
    //planner.synchronize();
    // 'S' for legacy compatibility. Should NOT BE USED for new development
    #if ENABLED(ACCELERATION_CONTROL)
      float acc_temp=0;
      if (parser.seenval('S'))
      {
         acc_temp=parser.value_linear_units();
         if(WITHIN(acc_temp, 0.1, default_accel))
         {
          planner.settings.travel_acceleration = planner.settings.acceleration = acc_temp;
         }
         else
         {
          planner.settings.travel_acceleration = planner.settings.acceleration = default_accel;
         }
      }
      if (parser.seenval('P'))
      {
         acc_temp=parser.value_linear_units();
         if(WITHIN(acc_temp, 0.1, default_accel))
         {
          planner.settings.acceleration = acc_temp;
         }
         else
         {
          planner.settings.acceleration = default_accel; 
         }
      }
      if (parser.seenval('R'))
      {
         acc_temp=parser.value_linear_units();
         if(WITHIN(acc_temp, 0.1, default_retract))
         {
          planner.settings.retract_acceleration = acc_temp;
         }
         else
         {
          planner.settings.retract_acceleration = default_retract; 
         }
      }
      if (parser.seenval('T'))
      {
         acc_temp=parser.value_linear_units();
         if(WITHIN(acc_temp, 0.1, default_travel))
         {
          planner.settings.travel_acceleration = acc_temp;
         }
         else
         {
          planner.settings.travel_acceleration = default_travel; 
         }
      }
    #else
      if (parser.seenval('S')) planner.settings.travel_acceleration = planner.settings.acceleration = parser.value_linear_units();
      if (parser.seenval('P')) planner.settings.acceleration = parser.value_linear_units();
      if (parser.seenval('R')) planner.settings.retract_acceleration = parser.value_linear_units();
      if (parser.seenval('T')) planner.settings.travel_acceleration = parser.value_linear_units();
    #endif
  }
}

/**
 * M205: Set Advanced Settings
 *
 *    B = Min Segment Time (µs)
 *    S = Min Feed Rate (units/s)
 *    T = Min Travel Feed Rate (units/s)
 *    X = Max X Jerk (units/sec^2)
 *    Y = Max Y Jerk (units/sec^2)
 *    Z = Max Z Jerk (units/sec^2)
 *    E = Max E Jerk (units/sec^2)
 *    J = Junction Deviation (mm) (If not using CLASSIC_JERK)
 */
void GcodeSuite::M205() {
  if (!parser.seen("BST" TERN_(HAS_JUNCTION_DEVIATION, "J") TERN_(HAS_CLASSIC_JERK, "XYZE"))) return;

  //planner.synchronize();
  if (parser.seenval('B')) planner.settings.min_segment_time_us = parser.value_ulong();
  if (parser.seenval('S')) planner.settings.min_feedrate_mm_s = parser.value_linear_units();
  if (parser.seenval('T')) planner.settings.min_travel_feedrate_mm_s = parser.value_linear_units();
  #if HAS_JUNCTION_DEVIATION
    #if HAS_CLASSIC_JERK && (AXIS4_NAME == 'J' || AXIS5_NAME == 'J' || AXIS6_NAME == 'J')
      #error "Can't set_max_jerk for 'J' axis because 'J' is used for Junction Deviation."
    #endif
    if (parser.seenval('J')) {
      const float junc_dev = parser.value_linear_units();
      if (WITHIN(junc_dev, 0.01f, 0.3f)) {
        planner.junction_deviation_mm = junc_dev;
        TERN_(LIN_ADVANCE, planner.recalculate_max_e_jerk());
      }
      else
        SERIAL_ERROR_MSG("?J out of range (0.01 to 0.3)");
    }
  #endif
  #if HAS_CLASSIC_JERK
    bool seenZ = false;
    LOGICAL_AXIS_CODE(
      if (parser.seenval('E')) planner.set_max_jerk(E_AXIS, parser.value_linear_units()),
      if (parser.seenval('X')) planner.set_max_jerk(X_AXIS, parser.value_linear_units()),
      if (parser.seenval('Y')) planner.set_max_jerk(Y_AXIS, parser.value_linear_units()),
      if ((seenZ = parser.seenval('Z'))) planner.set_max_jerk(Z_AXIS, parser.value_linear_units()),
      if (parser.seenval(AXIS4_NAME)) planner.set_max_jerk(I_AXIS, parser.value_linear_units()),
      if (parser.seenval(AXIS5_NAME)) planner.set_max_jerk(J_AXIS, parser.value_linear_units()),
      if (parser.seenval(AXIS6_NAME)) planner.set_max_jerk(K_AXIS, parser.value_linear_units())
    );
    #if HAS_MESH && DISABLED(LIMITED_JERK_EDITING)
      if (seenZ && planner.max_jerk.z <= 0.1f)
        SERIAL_ECHOLNPGM("WARNING! Low Z Jerk may lead to unwanted pauses.");
    #endif
  #endif // HAS_CLASSIC_JERK
}
