include <BOSL2/std.scad>
include <BOSL2/rounding.scad>
include <BOSL2/joiners.scad>
include <BOSL2/threading.scad>
include <NopSCADlib/core.scad>
include <NopSCADlib/vitamins/green_terminals.scad>
include <NopSCADlib/vitamins/pcbs.scad>


$fn = $preview ? 32 : 96;


//SLOP = 0.3;        // Slop factor for your printer, only applied on the camers screw mount
//diff("foo")
//cube([60,15,7],anchor=BOTTOM) {
//  tag("foo") 
//    position(LEFT) 
//    right(7.5) 
//    threaded_rod(d = 0.25 * INCH + SLOP, l = 15, pitch = INCH / 20, anchor = CENTER);
//
//  tag("foo") 
//    position(RIGHT) 
//    left(7.5) 
//    cylinder(d=7,h=15,anchor=CENTER);
//}



SMA_ANTENNA_HOLE_DIAM=7;
TOLERANCE=0.15; // Added or remove from parts that are designed to cut holes.

BITS=[1,2,4,8,16,32,64,128];

function bitwise_and
(
  v1,
  v2,
  bv = 1
) = ((v1 + v2) == 0) ? 0
  : (((v1 % 2) > 0) && ((v2 % 2) > 0)) ?
    bitwise_and(floor(v1/2), floor(v2/2), bv*2) + bv
  : bitwise_and(floor(v1/2), floor(v2/2), bv*2);
  // echo(bitwise_and(5,4));  
 
 
function vec2(p,d=[0,0]) = [p[0]!=undef?p[0]:d[0],p[1]!=undef?p[1]:d[1]];
function vec3(p,d=[0,0,0]) = [p[0]!=undef?p[0]:d[0],p[1]!=undef?p[1]:d[1],p[2]!=undef?p[2]:d[2]];
function vec4(p,d=[0,0,0,0]) = [p[0]!=undef?p[0]:d[0],p[1]!=undef?p[1]:d[1],p[2]!=undef?p[2]:d[2],p[3]!=undef?p[3]:d[3]];
function vec5(p,d=[0,0,0,0,0]) = [p[0]!=undef?p[0]:d[0],p[1]!=undef?p[1]:d[1],p[2]!=undef?p[2]:d[2],p[3]!=undef?p[3]:d[3],p[4]!=undef?p[4]:d[4]];
function vec6(p,d=[0,0,0,0,0,0]) = [p[0]!=undef?p[0]:d[0],p[1]!=undef?p[1]:d[1],p[2]!=undef?p[2]:d[2],p[3]!=undef?p[3]:d[3],p[4]!=undef?p[4]:d[4],p[5]!=undef?p[5]:d[5]];
function vec7(p,d=[0,0,0,0,0,0,0]) = [p[0]!=undef?p[0]:d[0],p[1]!=undef?p[1]:d[1],p[2]!=undef?p[2]:d[2],p[3]!=undef?p[3]:d[3],p[4]!=undef?p[4]:d[4],p[5]!=undef?p[5]:d[5],p[6]!=undef?p[6]:d[6]];




// side_support(bbox=false,l=10, cl=3, d=10, toPrint=1) show_anchors(std=true);

// side_support(1, l=10, cl=2,STARTSIZE=[0.01,0.1,0.1]);
// side_support(2);
// toPrint = 1 Side support over side
// toPrint = 2 Side support in corner
// toPrint = 3 Square (for lids or bottoms)
// toPrint = 4 Removal hole
// w = width
// l = length
// cl= top square size
// sideOfsset = forgot
module side_support(anchor=CENTER, spin=0, orient=UP, center, bbox=false, visual=true, toPrint=2, w=5, d=5, l=5, cl=0, screwDiam=2.2, sideOfsset=2.5, STARTSIZE=[0.01,0.01,0.01]) {
  
  o=[0,d/2-sideOfsset,0];
  ml=cl>0?l-cl:l/2;
  module support() {
    color([1,0.2,0.2]) {    
      
      if (toPrint==1) {
        hull() {
          translate([0,0,l]) cube([w,d,0.01]);
          translate([0,0,ml]) cube([w,d,0.01]);
          translate([0,0,0]) cube(STARTSIZE + [w,0,0]);
        }
      }
      if (toPrint==2) {
        hull() {
          translate([0,0,l]) cube([w,d,0.01]);
          translate([0,0,ml]) cube([w,d,0.01]);
          translate([0,0,0]) cube(STARTSIZE);
        }
      }
      if (toPrint==3) {
        hull() {
          cube([w,d,l]);
        }
      }  
    }
  }

  
  size=[w,d,l];

  anchors = [                 
    named_anchor("c", [0, 0 , size.z/2] + o, unit(UP)),
  ];  
  
  attachable(anchor, spin, orient, size, anchors=anchors) {
    union() {
      if (bbox) #cube(size, center=true);
      difference () {
        if (toPrint!=4) {
          translate(-size/2) render() support();
        }
        translate(o) cyl(d=screwDiam,l=100);
      }
    }
    children();
  }
  
}


// screw_mount(height=6);
module screw_mount(anchor=BOTTOM, spin=0, orient=UP, center, screw=2.5, height=5, d1=8) {    
  r1=d1/2;
  r2=(screw+1.5*2)/2;
  attachable(anchor, spin, orient, r1=r1, r2=r2, l=height) {
    difference() {
      cyl(h=height, d1=d1, d2=r2*2);
      up(0.5)
        cyl(h=height+2,d=screw*0.75);
    }
    children();
  }    
}


// openace_pcb( visual=true) {
//  show_anchors(std=false);
//}
module openace_pcb(anchor=BOTTOM, spin=0, orient=UP, center, bbox=false, rough=false, visual=true, pi=4) {
    size=[86.87,84,1.6];
    corner=[-size.x/2, -size.y/2, -size.z/2];

    positions=[
        [-80.9/2, -78/2, -size.z/2],
        [-80.9/2, 78/2, -size.z/2],
        [80.9/2, 78/2, -size.z/2],
        [80.9/2, -78/2, -size.z/2]
    ];
    
    xornerOff=3.5;
    
    terminals = [
      [0,22.7,1.6] + positions[0],
      [0,15.3,1.6] + positions[0],
      [0,7.43,1.6] + positions[0]
    ];
    
    PICO = [positions[1].x+31.6,positions[1].y+xornerOff,size.z/2];
    
    anchors = [                 
        named_anchor("mnt1", positions[0], [0,0,-1]),
        named_anchor("mnt2", positions[1], [0,0,-1]),
        named_anchor("mnt3", positions[2], [0,0,-1]),
        named_anchor("mnt4", positions[3], [0,0,-1]),

        named_anchor("right", [positions[2].x+xornerOff,0,size.z/2], unit(RIGHT)),

                
        named_anchor("ant1", [positions[2].x+xornerOff,positions[2].y-8.63,size.z/2+0.2], unit(RIGHT)),
        named_anchor("ant2", [positions[2].x+xornerOff,positions[2].y-29.6,size.z/2+0.2], unit(RIGHT)),
        named_anchor("adsb", [positions[2].x+xornerOff,positions[2].y-50.67,size.z/2+0.2], unit(RIGHT)),

        named_anchor("ant1_h", [positions[2].x+xornerOff,positions[2].y-8.63,size.z/2+6.5], unit(RIGHT)),
        named_anchor("ant2_h", [positions[2].x+xornerOff,positions[2].y-29.6,size.z/2+6.5], unit(RIGHT)),
        named_anchor("adsb_h", [positions[2].x+xornerOff,positions[2].y-50.67,size.z/2+6.5], unit(RIGHT)),

        named_anchor("gps",  [positions[2].x+xornerOff,positions[2].y-69,size.z/2+4.2], unit(RIGHT)),
        named_anchor("PICO", PICO + [0,0,2.5], unit(BACK)),
        named_anchor("POWER", [positions[0].x+15.05,positions[0].y-xornerOff,size.z/2+5.0], unit(FRONT)),
        named_anchor("FLASH", [positions[0].x+47.46,positions[0].y-xornerOff,size.z/2+0.5], unit(FRONT)),

        named_anchor("SWITCH", terminals[0]+[-xornerOff,0,2.5], unit(LEFT)),
        named_anchor("BAT", terminals[1]+[-xornerOff,0,2.5], unit(LEFT)),
        named_anchor("5VIN", terminals[2]+[-xornerOff,0,2.5], unit(LEFT)),


        ];  

    attachable(anchor, spin, orient, size=size+[0,0,0], center, anchors=anchors) {
      render(3) union() {
        if (visual) {    
              down(0.9) import("ImageToStl.com_openace.stl", convexity=2);
    
//            for ( mount = terminals ){
//              translate(mount)
//              rotate(180) green_terminal(gt_3p5, 2);
//            }
//            
//            translate(PICO) translate([0,-RPI_Pico[2]/2-2,0]) rotate(-90) pcb(RPI_Pico);
//
//          difference() {
//          cube(size, center=true);
//            for ( mount = positions ){
//              translate(mount)
//                 cyl(d=3.3,h=5);
//            }
//          }
        }
      }
      children();
    }
}


//ceramic_gps_antenna27(4,height=9,oversize=0,mountAngle=[0,0,0],flip=0);

//ceramic_gps_antenna27(1,height=9,oversize=0,mountAngle=[0,0,0],flip=0);
//  ceramic_gps_antenna27(4,height=5+(5+2),mountAngle=[0,0,0],flip=0,gpr=[45+90+180],gpd=(5+2));
// ceramic_gps_antenna27(5,height=24,mountAngle=[0,0,0]);

// ceramic_gps_antenna27(4,height=14.4);
// toPrint == 1 Print's the design
// toPrint == 2 prints the holes
// toPrint == 3 prints the plate only
// toPrint == 4 Mount (Not fully tested in all angles) oversize not used
module ceramic_gps_antenna27(toPrint=1, SCREW_DIAM=2.5, oversize=0, height=10, mountAngle=[0,0,0],flip=0,gpr=[],gpd=6) {
  HPOS=33/2;
  DIAM=38;

  // ANtenna Intself
  ant=27+oversize;
  antD2=ant/2;
  ant1=24+oversize;
  ant1D2=ant1/2;

  // Bottom Filter
  filt=26+oversize;
  filtD2=filt/2;   
    
  module mountHoles(holeDiam, height) {
    translate([HPOS,0,0])  cylinder(d=holeDiam*0.75, h=height,center=true);
    translate([0,HPOS,0])  cylinder(d=holeDiam*0.75, h=height,center=true);
    translate([-HPOS,0,0]) cylinder(d=holeDiam*0.75, h=height,center=true);
    translate([0,-HPOS,0]) cylinder(d=holeDiam*0.75, h=height,center=true);
  }

  translate([0,0,height]) rotate(mountAngle+[180*flip,0,0]) translate([0,0,-0.8*flip]){
    if (toPrint == 3) {
        color([0,0.6,0]) 
          cylinder(d=DIAM, h=0.8);
    }  
    
    if (toPrint == 1) {
      difference() {
        color([0,0.6,0]) cylinder(d=DIAM, h=0.8);
        mountHoles(SCREW_DIAM, 4);
      }
      
      // Antenna
      color([0.8,0.7,0.6])    translate([-antD2,-antD2,0.8]) cube([ant,ant,7 + 0.8 + (oversize>0?1:0)]);
      color([0.98,0.98,0.98]) translate([-ant1D2,-ant1D2,0.8]) cube([ant1,ant1,7 + 0.9 + (oversize>0?1:0)]);
      color([0.88,0.88,0.88]) translate([3,0,0.8]) cylinder(d=1.5,h=7 + 1.2);
      color([0.88,0.88,0.88]) translate([0,3,0.8]) cylinder(d=1.5,h=7 + 1.2);
      color([0.88,0.88,0.88]) translate([0,0,0.8]) cylinder(d=1,h=7 + 0.91);

      // Filter (Bottom)
      color([0.7,0.7,0.7]) translate([-filtD2,-filtD2,-3-(oversize>0?0.8:0)]) cube([filt,filt,3 + (oversize>0?0.8:0)]);
    }

    if (toPrint == 2) {
        mountHoles(SCREW_DIAM, 18);
    }
    
    if (toPrint == 5) {
        for (i = [0:3]) {
//          rotate([0,0,45+i*90]) translate([filtD2,0,0]) rotate([0,-60,0]) cylinder(d=5,l=25,center=true);
          rotate([0,0,45+i*90]) 
          translate([filtD2,0,0]) 
          rotate([0,-45,0]) 
          translate([5,0,0]) 
          cube([10,4,30],center=true);
        }
    }
  }

  if (toPrint==4) {
    difference() {
      crop([100,100,100], [0,0,height], mountAngle) 
        linear_extrude(50) 
        projection(cut = false) 
        rotate(mountAngle) 
        ceramic_gps_antenna27(3,height=0, oversize=0.5, mountAngle=[0,0,0],flip=flip);
      union() {
         ceramic_gps_antenna27(1,2,oversize=1,height=height,mountAngle=mountAngle,flip=flip);
         ceramic_gps_antenna27(2,SCREW_DIAM,0,height=height,mountAngle=mountAngle,flip=flip);
         ceramic_gps_antenna27(5,SCREW_DIAM,0,height=height,mountAngle=mountAngle,flip=0);         
         for ( r = gpr ){
           rotate( [0, 0, r])
             translate([60/2+gpd/2, 0, height/2])
             difference() {
               cylinder(d=60+gpd*2,height+2,center=true);
               cylinder(d=60,height+1,center=true);
             }
        }
      }
    }
  }
}

// Simple cropping function
// c=Size of cropping cube
// t=Location of cropping cube
// r=Rotation angle of cropping cube
module crop(c,t,r)
  difference() {
      children();
      translate(t) rotate(r) translate([-c.x/2,-c.y/2,0]) cube(c);
  }

  
  
  
