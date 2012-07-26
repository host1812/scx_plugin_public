#!/bin/sh
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
. scx_base.sh

GENCERT_CONFIG_FILE=scx-seclevel1.conf
GENCERT_CERT_FILE=scx-seclevel1.pem
GENCERT_KEY_FILE=scx-seclevel1-key.pem

_write_cn() {
    echo "$2.CN=$3" >> $1
}

_write_dc() {
    filename=$1
    shift; shift
    domainparts=`echo "$*" | xargs -n 1 echo | rev | xargs | rev`
    counter=0
    for i in $domainparts
    do
      echo "$counter.DC=$i" >> $filename
      counter=`expr $counter + 1`
    done
}


## Generate config, certificate and key files in directory given as argument
generate_certificate() {
    
    SSL_DIR=$1
    
    if [ ! "$SSL_DIR" = "" ] && [ -d $SSL_DIR ]; then
        ## Create new file
        echo "" > $SSL_DIR/$GENCERT_CONFIG_FILE
        
        ## Identify OS and use OS information to retrieve Hostname information
        os_identification
        hostname_identification $OS_NAME $OS_DISTRIBUTION
	
        ## Write basic req information
        echo "[ req ]" >> $SSL_DIR/$GENCERT_CONFIG_FILE
        echo "distinguished_name=req_dn" >> $SSL_DIR/$GENCERT_CONFIG_FILE
        echo "prompt=no"  >> $SSL_DIR/$GENCERT_CONFIG_FILE
        echo "[ req_dn ]" >> $SSL_DIR/$GENCERT_CONFIG_FILE
        
        ## Write DC and CN
        _write_dc $SSL_DIR/$GENCERT_CONFIG_FILE `echo $HOST_FQDN | sed 's/\./ /g'`
        _write_cn $SSL_DIR/$GENCERT_CONFIG_FILE 0 $HOST_NAME
        _write_cn $SSL_DIR/$GENCERT_CONFIG_FILE 1 $HOST_FQDN
        
        ## Protect cert file
        if [ -f $SSL_DIR/$GENCERT_CONFIG_FILE ]; then
            chown root.root $SSL_DIR/$GENCERT_CONFIG_FILE
            chmod 400 $SSL_DIR/$GENCERT_CONFIG_FILE
        fi
	
	## Note - this script does not preserve existing files!
	
	/usr/bin/openssl req \
	    -x509 -days 3650 -newkey rsa:2048 -nodes \
	    -config $SSL_DIR/$GENCERT_CONFIG_FILE \
	    -keyout $SSL_DIR/$GENCERT_KEY_FILE \
	    -out    $SSL_DIR/$GENCERT_CERT_FILE \
	    2>>/dev/stderr
	
	chmod 400 $SSL_DIR/*
	chmod 444 $SSL_DIR/$GENCERT_CERT_FILE
	return 0
    else
	echo "generate_certificate() : directory given as argument does not exist ($1)"
	return 1
    fi
}

