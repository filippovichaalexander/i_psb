// i_psb parametric case — M0 scaffold, not a final mechanical design.
// Units: millimetres. Print in PETG, 0.2 mm layers, 3 perimeters.
// Read docs/14-mechanical.md before printing. Antenna keep-out is not optional.

part = "both";          // "front", "back", "both"
show_components = true; // translucent component preview, not printed

case_w = 80.0;
case_h = 76.0;
case_d = 18.0;
wall = 2.0;
radius = 4.0;
front_d = 6.0;
back_d = case_d - front_d - 0.2;

screen_w = 50.6;        // visible 48.55 x 23.70 + 1 mm clearance
screen_h = 25.7;
screen_y = 15.0;        // centre offset from case centre

wheel_d = 40.0;
wheel_y = -18.0;

usb_w = 9.5;
usb_h = 4.0;
jack_d = 6.5;
switch_w = 9.5;
switch_h = 4.5;
boss_inset = 6.0;
boss_d = 5.0;
screw_clear_d = 2.7;
screw_pilot_d = 2.2;

$fn = 64;

module rounded_rect(w, h, r) {
    hull() {
        for (x = [-w / 2 + r, w / 2 - r])
            for (y = [-h / 2 + r, h / 2 - r])
                translate([x, y]) circle(r = r);
    }
}

module rounded_box(w, h, d, r) {
    linear_extrude(height = d, center = false)
        rounded_rect(w, h, r);
}

module screw_positions() {
    for (x = [-1, 1], y = [-1, 1])
        translate([x * (case_w / 2 - boss_inset), y * (case_h / 2 - boss_inset)])
            children();
}

module side_cutouts() {
    // USB-C charging + data slots on the bottom edge.
    translate([-usb_w / 2, -case_h / 2 - 1, 1.0])
        cube([usb_w, wall + 2, usb_h]);
    // Power switch on the right edge.
    translate([case_w / 2 - 1, -10.0, 1.0])
        cube([wall + 2, switch_w, switch_h]);
    // 3.5 mm jack through the top edge.
    translate([0, case_h / 2, 10.0])
        rotate([90, 0, 0])
            cylinder(d = jack_d, h = wall + 2, center = true);
}

module case_front() {
    difference() {
        union() {
            rounded_box(case_w, case_h, front_d, radius);
            // A shallow lip helps locate the back shell; adjust after a test print.
            translate([0, 0, front_d - 0.8])
                difference() {
                    rounded_box(case_w - 0.4, case_h - 0.4, 0.8, radius - 0.2);
                    translate([wall, wall, -0.1])
                        rounded_box(case_w - 2 * wall - 0.4, case_h - 2 * wall - 0.4,
                                    1.0, max(0.5, radius - wall));
                }
        }

        // Cavity from the inside of the front face.
        translate([0, 0, wall])
            rounded_box(case_w - 2 * wall, case_h - 2 * wall, front_d,
                        max(0.5, radius - wall));

        // Display window.
        translate([-screen_w / 2, screen_y - screen_h / 2, -1])
            cube([screen_w, screen_h, wall + 2]);

        // Wheel opening.
        translate([0, wheel_y, -1])
            cylinder(d = wheel_d, h = wall + 2);

        // Screw clearance holes.
        screw_positions()
            translate([0, 0, -1])
                cylinder(d = screw_clear_d, h = front_d + 2);

        side_cutouts();
    }
}

module case_back() {
    difference() {
        union() {
            difference() {
                rounded_box(case_w, case_h, back_d, radius);
                translate([0, 0, wall])
                    rounded_box(case_w - 2 * wall, case_h - 2 * wall, back_d,
                                max(0.5, radius - wall));
            }
            // M2.5 bosses; use brass inserts for repeated assembly.
            screw_positions()
                translate([0, 0, wall])
                    cylinder(d = boss_d, h = back_d - wall);
        }

        // Pilot holes for the screws.
        screw_positions()
            translate([0, 0, wall])
                cylinder(d = screw_pilot_d, h = back_d - wall + 0.2);

        side_cutouts();
    }
}

module component_preview() {
    % translate([0, -8, 2]) cube([40, 20, 6], center = true);        // Li-Po
    % translate([-18, 20, 2]) cube([23.5, 18, 3.5], center = true);  // ESP32
    % translate([16, 20, 2]) cube([25, 22, 3.5], center = true);     // microSD
    % translate([-16, -10, 2]) cube([32, 17, 3.2], center = true);   // PCM5102A
    % translate([18, -10, 2]) cube([20, 23, 3.0], center = true);    // MAX97220
    % translate([0, -30, 2]) cube([35, 15, 3.0], center = true);     // TTP229
    % translate([0, 30, 2]) cube([72, 30, 6.0], center = true);      // EPD board
    // Antenna keep-out: keep metal and battery out of this corner.
    % translate([case_w / 2 - 10, case_h / 2 - 10, 0.2])
        cylinder(d = 20, h = 0.4, center = true);
}

if (part == "front" || part == "both") {
    case_front();
}
if (part == "back" || part == "both") {
    translate([0, 0, case_d + 4]) case_back();
}
if (show_components) {
    component_preview();
}
