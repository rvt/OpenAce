include <BOSL2/std.scad>
include <BOSL2/rounding.scad>
include <BOSL2/joiners.scad>
include <BOSL2/threading.scad>
include <BOSL2/modular_hose.scad>
include <BOSL2/hinges.scad>


$fn = $preview ? 32 : 128;

// Diameter of the inner case
DIAMETER=39;

WALL=0.4*4;

BHEIGHT=7; // [5:15]

// Height of the top screw case
THEIGHT=15;  // [5:25]

// CLearance of the top antenna, keep at least 1mm as teh ceramic antenna needs that
ANTENNA_CLERARANCE=1.5; // [1,1.5,2,2.5,3]

/** Visualisation only **/
// hangle of the hinge
HINGE_ANGLE=45; // [0:110]

// Position of the top to move it up/down
CAP_POSITION=20; // [-10:25]

// Visualize cropping
CROP=false; 


/* [Hidden] */
PITCH=2;
_DIAMETER=DIAMETER+PITCH+WALL*2;
HINGE_LENGTH=6;
HINGE_SEGS=5;
_CROP=CROP && $preview;



// Simple cropping function
// c=Size of cropping cube
// t=Location of cropping cube
// r=Rotation angle of cropping cube
module crop(c=100,t=[0,0,0],r=0)
  difference() {
      children();
      translate(t) 
      rotate(r) 
      cube([c,c,c], anchor=LEFT);
  }


crop(c=(_CROP?300:0),t=[0,0,0],r=90)
{  

//***Place here nay mode you want to visaluze and see if it fits

// TOPGNSS dual band cermaic antenna
%cyl(d=39,l=0.6,  anchor=BOTTOM) {
 position(TOP) cuboid([28,28,7],anchor=BOTTOM);
 position(BOTTOM) cuboid([25,25,4],anchor=TOP);
}

//***Place here nay mode you want to visaluze and see if it fits

// Bottom
diff()
trapezoidal_threaded_rod(d=_DIAMETER, l=BHEIGHT, end_len1=1.5, flank_angle=30, pitch=2, anchor=TOP) {

  tag("remove") {
    attach(TOP) up(WALL) cyl(d=DIAMETER,h=BHEIGHT+0.01,anchor=TOP);

    position(BOTTOM+RIGHT) cube([10,3,WALL],anchor=BOTTOM+RIGHT);
    
  }
  left(HINGE_LENGTH*2) attach(BOTTOM) right(HINGE_LENGTH) knuckle_hinge(length=HINGE_LENGTH*HINGE_SEGS, segs=HINGE_SEGS, offset=2.2, arm_height=0, anchor=BOT+RIGHT, inner=true);
}

// Cap
up($preview?CAP_POSITION:THEIGHT+10)
diff()
cyl(d=_DIAMETER+WALL,h=THEIGHT, rounding2=3, anchor=BOTTOM) {
  
  attach(TOP) down(0.5) {  
    tag("remove")  {
      cylinder(d=12,h=ANTENNA_CLERARANCE+1, anchor=TOP);
      down(ANTENNA_CLERARANCE+1) trapezoidal_threaded_rod(d=_DIAMETER, l=THEIGHT, flank_angle=30, pitch=2, internal=true, anchor=TOP);
    }
  }
}


// Hinge 
LENGTH=_DIAMETER/2+2+20;
WIDTH=14;
down(2.2+BHEIGHT+($preview?0:10))
xrot(($preview?-HINGE_ANGLE:0))
left(HINGE_LENGTH*3) back(2) xrot(90) zrot(-90) diff()
prismoid(size1=[2,LENGTH], size2=[4,LENGTH], shift=[1,0], h=WIDTH, anchor=FRONT+TOP) {
     {
      tag("keep") position(TOP+FRONT) knuckle_hinge(length=HINGE_LENGTH*HINGE_SEGS, segs=HINGE_SEGS, offset=2, inner=false, arm_angle=90, anchor=BOT+RIGHT, orient=UP, spin=-90,knuckle_diam=4) {
      attach(BACK) xrot(20) down(1) cuboid([HINGE_LENGTH-1,2.5,2], anchor=BOTTOM);
      }

      tag("remove") position(TOP+FRONT) knuckle_hinge(length=HINGE_LENGTH*HINGE_SEGS, segs=HINGE_SEGS, offset=2.25, 
         inner=false, arm_angle=90, anchor=BOT+RIGHT, orient=UP, spin=-90, knuckle_clearance=1,knuckle_diam=4.5);
    }


    // position(LEFT+BACK) cube([10,WALL,WIDTH], anchor=RIGHT+BACK);
    diff()
    position(LEFT+BACK) cube([12,WALL,WIDTH], anchor=LEFT+BACK) {
      right(WALL) position(LEFT+FRONT) prismoid(size1=[WIDTH/2,WALL], size2=[0,WALL], shift=[-WIDTH/4,0], h=WIDTH/2, anchor=BOTTOM+LEFT, orient=FRONT);
      
      tag("remove") {
        up(4) right(2) attach(FRONT) cyl(d=3, l=10) {
          position(TOP) cyl(d=6, l=2);
        }
        down(4) right(0) attach(FRONT) cyl(d=3, l=10) {
          position(TOP) cyl(d=6, l=2);
        }
      }
    }
    
    //position(TOP+FRONT) #attach(RIGHT) cuboid([4,4,3], anchor=BOTTOM+LEFT+BACK);

//    #position(RIGHT) cuboid([10,2,5], anchor=BOTTOM);
 }
}
  
