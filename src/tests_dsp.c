//---------------------------------------------
// ## @Author: Med
// ## @Editor: Emacs - ggtags
// ## @TAGS:   Global
// ## @CPU:    TEST PLATFORM FOR FIRMWARE
// ##
// #### TESTS_DSP.C ###########################
//---------------------------------------------

// Includes Modules for tests --------------------------------------------------
#include "dsp.h"

// Auxiliary tests modules -----------------------------------------------------
#include "tests_ok.h"

#include <stdio.h>
#include <math.h>


// Types Constants and Macros --------------------------------------------------


// Externals -------------------------------------------------------------------



// Globals ---------------------------------------------------------------------



// Module Functions to Test ----------------------------------------------------
void TEST_Dsp_Module (void);


// Module Functions ------------------------------------------------------------
int main (int argc, char *argv[])
{
    printf("Simple dsp module tests\n");
    TEST_Dsp_Module();
    
    return 0;
}


void TEST_Dsp_Module (void)
{
    printf("Testing dsp module\n");
    int errors = 0;
    
    pid_data_obj_t pid1;

    pid1.kp = 128;
    pid1.ki = 0;
    pid1.kd = 0;
    PID_Flush_Errors(&pid1);

    pid1.setpoint = 100;
    pid1.sample = 100;
    short d = 0;

    printf("Test PID only Kp: ");
    d = PID(&pid1);
    if (d != 0)
    {
        errors = 1;
        PrintERR();
        printf("expected 0, d was: %d\n", d);
    }

    pid1.sample = 99;
    d = PID(&pid1);
    if (d != 1)
    {
        errors = 1;        
        PrintERR();
        printf("expected 1, d was: %d\n", d);
    }

    pid1.sample = 0;
    d = PID(&pid1);
    if (d != 100)
    {
        errors = 1;        
        PrintERR();
        printf("expected 100, d was: %d\n", d);
    }

    if (!errors)
        PrintOK();
    

    printf("Test PID only Ki: ");
    errors = 0;
    pid1.kp = 0;
    pid1.ki = 64;
    PID_Flush_Errors(&pid1);

    pid1.setpoint = 100;
    pid1.sample = 0;

    d = PID(&pid1);
    if (d != 50)
    {
        errors = 1;        
        PrintERR();
        printf("expected 50, d was: %d\n", d);
    }

    d = PID(&pid1);
    if (d != 100)
    {
        errors = 1;        
        PrintERR();
        printf("expected 100, d was: %d\n", d);
    }

    if (!errors)
        PrintOK();

    printf("Test PID only Kd positives: ");
    errors = 0;
    pid1.kp = 0;
    pid1.ki = 0;
    pid1.ki = 64;    
    PID_Flush_Errors(&pid1);

    pid1.setpoint = 100;
    pid1.sample = 0;

    d = PID(&pid1);
    if (d != 50)
    {
        errors = 1;        
        PrintERR();
        printf("expected 50, d was: %d\n", d);
    }

    d = PID(&pid1);
    if (d != 100)
    {
        errors = 1;        
        PrintERR();
        printf("expected 100, d was: %d\n", d);
    }

    if (!errors)
        PrintOK();

    printf("Test PID only Kd negatives: ");
    errors = 0;
    pid1.kp = 0;
    pid1.ki = 0;
    pid1.ki = 64;    
    PID_Flush_Errors(&pid1);

    pid1.setpoint = 100;
    pid1.sample = 200;

    d = PID(&pid1);
    if (d != -50)
    {
        errors = 1;        
        PrintERR();
        printf("expected -50, d was: %d\n", d);
    }

    d = PID(&pid1);
    if (d != -100)
    {
        errors = 1;        
        PrintERR();
        printf("expected -100, d was: %d\n", d);
    }

    if (!errors)
        PrintOK();
    
}




//--- end of file ---//


