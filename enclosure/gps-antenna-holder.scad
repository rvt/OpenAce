include <BOSL2/std.scad>
include <BOSL2/rounding.scad>
include <BOSL2/joiners.scad>
include <BOSL2/threading.scad>
include <BOSL2/modular_hose.scad>
include <BOSL2/hinges.scad>


$fn = $preview ? 32 : 256;

// Diameter of the inner case
DIAMETER=37;

WALL=0.4*5;

BHEIGHT=6; // [5:0.5:15]

// Height of the top screw case
THEIGHT=19;  // [5:25]

// Clearance of the top antenna. L
// eep at least 1mm as the ceramic antenna needs that
ANTENNA_CLERARANCE=1; // [1,1.5,2,2.5,3]

// Friction makes some extra room for a rubber ring to ensure the antenna will stay in position during vibrations
FRICTION_RING=true;

/** [Visualisation only] **/
// hangle of the hinge
HINGE_ANGLE=45; // [0:110]

// Position of the top to move it up/down
CAP_POSITION=20; // [-7.5:0.5:25]

// Visualize cropping
CROP=false; 

MODULE="TOPGNSS"; // [TOPGNSS, square20_20_5, No Module]

/* [Hidden] */
_DIAMETER=DIAMETER+(0.4*4)*2;
HINGE_LENGTH=6;
HINGE_SEGS=5;
_CROP=CROP && $preview;
KNUNCKLE_DIAM=6;
NOTCHES=[0,120,240];
CAP_BOTTOM_SLACK=0.5;


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


//*** START Place here your visalisation modules
module topGNSS() {
  %cyl(d=39,l=0.6,  anchor=BOTTOM) {
    position(TOP) cuboid([28,28,7],anchor=BOTTOM);
    position(BOTTOM) cuboid([25,25,4],anchor=TOP);
  }
}
//***STOP Place here your visalisation modules


crop(c=(_CROP?300:0),t=[0,0,0],r=90)
{  

//*** START Place here your visalisation 
  if (MODULE == "TOPGNSS") topGNSS();
  if (MODULE == "square20_20_5") %cuboid([20,20,5], anchor=TOP);
//***STOP Place here your visalisation 



// Cap
color("#FFA0A0")
up($preview?CAP_POSITION:THEIGHT)
diff()
cyl(d=_DIAMETER+WALL+CAP_BOTTOM_SLACK,h=THEIGHT, rounding2=3, anchor=BOTTOM) {
  
  attach(TOP) down(0.5) {  
    tag("remove")  {
      cylinder(d=12,h=ANTENNA_CLERARANCE, anchor=TOP);
      down(ANTENNA_CLERARANCE) cyl(d=_DIAMETER+0.5,h=THEIGHT, anchor=TOP, rounding2=2);
    }
  }

  tag("remove") attach(BOTTOM) down(1.5)
    for (n = NOTCHES) {
      zrot(n+60) left(_DIAMETER/2) cuboid([12,4,4], anchor=TOP);
    }
}

// Bottom
diff()
cyl(d=_DIAMETER, l=BHEIGHT, anchor=TOP) {

  tag("remove") {
    attach(TOP) up(1) cyl(d=DIAMETER,h=BHEIGHT+0.01,anchor=TOP);
    // Gap bottom
    position(BOTTOM+RIGHT) cube([10,3,WALL],anchor=BOTTOM+RIGHT);    
  }
  
  
  attach(BOTTOM)
  for (n = NOTCHES) {
    zrot(n + 60) left(_DIAMETER/2-0.25)  zrot(90) wedge([3, 1+CAP_BOTTOM_SLACK, 5], anchor=BOTTOM+FRONT, orient=DOWN);
  }

}


// Bottom plate
PLATE_DOWN=0;
down(BHEIGHT+PLATE_DOWN)
diff()
cyl(d2=DIAMETER-10, d1=DIAMETER-12, l=0.2*4, anchor=TOP) {
  left(HINGE_LENGTH*2) attach(BOTTOM) right(HINGE_LENGTH) knuckle_hinge(length=HINGE_LENGTH*HINGE_SEGS, segs=HINGE_SEGS, offset=KNUNCKLE_DIAM/2+0.2, arm_height=0, anchor=BOT+RIGHT, inner=true,knuckle_diam=KNUNCKLE_DIAM);
      
}



// Hinge 
LENGTH=_DIAMETER/2+2+20;
WIDTH=14;
down(BHEIGHT+PLATE_DOWN+($preview?3.5:15))
xrot(($preview?-HINGE_ANGLE:0))
left(HINGE_LENGTH*3) back(3) xrot(90) zrot(-90) diff()
prismoid(size1=[KNUNCKLE_DIAM/8,LENGTH], size2=[KNUNCKLE_DIAM,LENGTH], shift=[(KNUNCKLE_DIAM - KNUNCKLE_DIAM/8)/2,0], h=WIDTH, anchor=FRONT+TOP) {
     
     
    position(TOP+FRONT) knuckle_hinge(length=HINGE_LENGTH*HINGE_SEGS, segs=HINGE_SEGS, inner=false, arm_angle=90, anchor=BOT+RIGHT, orient=UP, spin=-90,knuckle_diam=KNUNCKLE_DIAM, offset=KNUNCKLE_DIAM/2) {

        attach(BACK) xrot(20) down(1) cuboid([HINGE_LENGTH-1,2.5,2], anchor=BOTTOM);
      
        if (FRICTION_RING) {
          tag("remove") right(HINGE_LENGTH) attach(LEFT) {
            color("orange") %torus(od=6,id=2, anchor=BOTTOM);
            cylinder(d=7,h=1.75);
          }
          
        }

      }

      position(TOP+FRONT) knuckle_hinge(length=HINGE_LENGTH*HINGE_SEGS, segs=HINGE_SEGS, offset=KNUNCKLE_DIAM/2, inner=false, arm_angle=90, anchor=BOT+RIGHT, orient=UP, spin=-90, knuckle_clearance=1,knuckle_diam=KNUNCKLE_DIAM) {
      

      }



    // #position(LEFT+BACK) cube([10,WALL,WIDTH], anchor=RIGHT+BACK); on top..
    position(LEFT+BACK) cube([12,3,WIDTH+8], anchor=LEFT+BACK) {
      right(WALL) position(LEFT+FRONT) prismoid(size1=[WIDTH/2,WALL], size2=[0,WALL], shift=[-WIDTH/4,0], h=WIDTH/2, anchor=BOTTOM+LEFT, orient=FRONT);
      
      tag("remove") {
        up(4) right(2) attach(FRONT) cyl(d=3, l=10) {
          %position(TOP) cyl(d=6, l=2);
        }
        down(4) right(-1) attach(FRONT) cyl(d=3, l=10) {
          %position(TOP) cyl(d=6, l=2);
        }
      }
    }
    
    //position(TOP+FRONT) #attach(RIGHT) cuboid([4,4,3], anchor=BOTTOM+LEFT+BACK);

//    #position(RIGHT) cuboid([10,2,5], anchor=BOTTOM);
 }
}
  
