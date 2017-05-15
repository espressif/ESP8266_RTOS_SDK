# driver_lib
## method 1
Generate libdriver.a in SDK/lib, in driver_lib folder, run:
 
    ./make_lib.sh driver

## method 2
* STEP 1: 
    
    Copy driver folder to your project sub-folder, such as app folder. Unused drivers can be removed in your project.

* STEP 2: 

    Modify Makefile in app folder.

    1). Search SUBDIRS, add driver as subdir, such as:

        SUBDIRS = user driver

    2). Search COMPONENTS_eagle.app.v6, add libdriver.a, such as:

        COMPONENTS_eagle.app.v6 = user/libuser.a driver/libdriver.a