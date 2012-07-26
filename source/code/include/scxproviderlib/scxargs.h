/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file
 
    \brief     Definition of SCXProviderLib::SCXArgs
 
    \date      07-07-25 09:31:04
 

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXARGS_H
#define SCXARGS_H

#include <scxproviderlib/scxinstance.h>

namespace SCXProviderLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Representation of a colletion of arguments.
       
       Arguments are a colletion of typed name/value pairs. 
       
       For now implemented as a typedef of SCXInstance. When used as SCXArgs the
       \<op\>Key() functions shall not be used as they have no meaning in that 
       context.
       
    */
    typedef SCXInstance SCXArgs;
}  


#endif /* SCXARGS_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
