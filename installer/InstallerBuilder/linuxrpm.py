# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to create a Linux RPM file
#
# Date:   2007-10-08 10:42:48
#

import os
import scxutil
from installer import Installer
from scriptgenerator import Script

##
# Class containing logic for creating a Linux RPM file
#
class LinuxRPMFile(Installer):
    ##
    # Ctor.
    # \param[in] srcDir Absolute path to source directory.
    # \param[in] targetDir Absolute path to target directory.
    # \param[in] installerDir Absolute path to installer directory.
    # \param[in] configuration Configuration map.
    #
    def __init__(self, srcDir, targetDir, installerDir, intermediateDir, configuration):
        Installer.__init__(self, srcDir, targetDir, installerDir, intermediateDir, configuration)
        self.specFileName = os.path.join(self.tempDir, 'scx.spec')
        self.preInstallPath = os.path.join(self.tempDir, "preinstall.sh")
        self.postInstallPath = os.path.join(self.tempDir, "postinstall.sh")
        self.preUninstallPath = os.path.join(self.tempDir, "preuninstall.sh")

    ##
    # Generate the package description files (e.g. prototype file)
    #
    def GeneratePackageDescriptionFiles(self):
        self.GenerateSpecFile()

    ##
    # Generate pre-, post-install scripts and friends
    #
    def GenerateScripts(self):
        preInstall = Script(self.preInstallPath, self.configuration)
        preInstall.WriteLn('if [ $1 -eq 2 ]; then')
        # If this is an upgrade then remove the services.
        preInstall.CallFunction(preInstall.StopWSManService())
        preInstall.CallFunction(preInstall.StopPegasusService())
        preInstall.CallFunction(preInstall.RemoveWSManService())
        preInstall.CallFunction(preInstall.RemovePegasusService())
        preInstall.WriteLn('fi')
        preInstall.WriteLn('exit 0')
        preInstall.Generate()
        
        postInstall = Script(self.postInstallPath, self.configuration)
        postInstall.WriteLn('set -e')
        postInstall.CallFunction(postInstall.CreateSoftLinkToSudo())
        postInstall.CallFunction(postInstall.WriteInstallInfo())
        postInstall.CallFunction(postInstall.GenerateCertificate())
        postInstall.WriteLn('set +e')
        postInstall.WriteLn('if [ $1 -eq 1 ]; then')
        # If this is a fresh install and not an upgrade
        postInstall.CallFunction(postInstall.ConfigurePAM())
        postInstall.CallFunction(postInstall.ConfigureRunas())
        postInstall.WriteLn('fi')
        postInstall.WriteLn('set -e')
        postInstall.CallFunction(postInstall.ConfigurePegasusService())
        postInstall.CallFunction(postInstall.StartPegasusService())
        postInstall.WriteLn('set +e')
        postInstall.CallFunction(postInstall.RegisterExtProviders())
        postInstall.WriteLn('exit 0')
        postInstall.Generate()

        preUninstall = Script(self.preUninstallPath, self.configuration)
        preUninstall.WriteLn('if [ $1 -eq 0 ]; then')
        # If this is a clean uninstall and not part of an upgrade
        preUninstall.CallFunction(preUninstall.StopPegasusService())
        preUninstall.CallFunction(preUninstall.RemovePegasusService())
        preUninstall.CallFunction(preUninstall.UnconfigurePAM())
        preUninstall.CallFunction(preUninstall.RemoveAdditionalFiles())
        preUninstall.CallFunction(preUninstall.DeleteSoftLinkToSudo())
        preUninstall.WriteLn('fi')
        preUninstall.WriteLn('exit 0')
        preUninstall.Generate()
        
    ##
    # Creates the specification file used for packing.
    #
    def GenerateSpecFile(self):
        specfile = open(self.specFileName, 'w')

        specfile.write('%define __find_requires %{nil}\n')
        specfile.write('%define _use_internal_dependency_generator 0\n\n')

        specfile.write('Name: ' + self.configuration['short_name'] + '\n')
        specfile.write('Version: ' + self.configuration['version'] + '\n')
        specfile.write('Release: ' + self.configuration['release'] + '\n')
        specfile.write('Summary: ' + self.configuration['long_name'] + '\n')
        specfile.write('Group: Applications/System\n')
        specfile.write('License: ' + self.configuration['license'] + '\n')
        specfile.write('Vendor: ' + self.configuration['vendor'] + '\n')
        if self.configuration['pfdistro'] == 'SUSE':
            if self.configuration['pfmajor'] == 11:
            	specfile.write('Requires: glibc >= 2.9-7.18, openssl >= 0.9.8h-30.8, pam >= 1.0.2-17.2, insserv >= 1.12.0-25.1\n')
            elif self.configuration['pfmajor'] == 10:
            	specfile.write('Requires: glibc >= 2.4-31.30, openssl >= 0.9.8a-18.15, pam >= 0.99.6.3-28.8, insserv >= 1.04.0-20.13\n')
            elif self.configuration['pfmajor'] == 9:
            	specfile.write('Requires: glibc >= 2.3.3-98.28, libstdc++-41 >= 4.1.2, libgcc-41 >= 4.1.2, openssl >= 0.9.7d-15.10, pam >= 0.77-221.1, insserv >= 1.00.2-85.1\n')
            else:
                raise PlatformNotImplementedError(self.configuration['pfdistro'] + self.configuration['pfmajor']) 
        elif self.configuration['pfdistro'] == 'REDHAT':
            if self.configuration['pfmajor'] == 6:
                specfile.write('Requires: glibc >= 2.12-1.7, openssl >= 1.0.0-4, pam >= 1.1.1-4, redhat-lsb >= 4.0-2.1\n')
            elif self.configuration['pfmajor'] == 5:
                specfile.write('Requires: glibc >= 2.5-12, openssl >= 0.9.8b-8.3.el5, pam >= 0.99.6.2-3.14.el5, redhat-lsb >= 3.1-12.2\n')
            elif self.configuration['pfmajor'] == 4:
                specfile.write('Requires: glibc >= 2.3.4-2, openssl >= 0.9.7a-43.1, pam >= 0.77-65.1, redhat-lsb >= 1.3-5.2\n')
            else:
                raise PlatformNotImplementedError(self.configuration['pfdistro'] + self.configuration['pfmajor']) 
        else:
            raise PlatformNotImplementedError(self.configuration['pfdistro'])
        specfile.write('Provides: cim-server\n')
        specfile.write('Conflicts: %{name} < %{version}-%{release}\n')
        specfile.write('Obsoletes: %{name} < %{version}-%{release}\n')
        specfile.write('%description\n')
        specfile.write(self.configuration['description'] + '\n')
        
        specfile.write('%files\n')

        # Now list all files in staging directory
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'sysdir':
                pass
            else:
                specfile.write('%defattr(' + str(stagingObject.GetPermissions()) + \
                               ',' + stagingObject.GetOwner() + \
                               ',' + stagingObject.GetGroup() + ')\n')
                if stagingObject.GetFileType() == 'dir':
                    specfile.write('%dir /' + stagingObject.GetPath() + '\n')
                elif stagingObject.GetFileType() == 'conffile':
                    specfile.write('%config /' + stagingObject.GetPath() + '\n')
                else:
                    specfile.write('/' + stagingObject.GetPath() + '\n')

        preinstall = open(self.preInstallPath, 'r')
        postinstall = open(self.postInstallPath, 'r')
        preuninstall = open(self.preUninstallPath, 'r')
        specfile.write('%pre\n')
        specfile.write(preinstall.read())
        specfile.write('%post\n')
        specfile.write(postinstall.read())
        specfile.write('%preun\n')
        specfile.write(preuninstall.read())
        

    ##
    # Create the RPM directive file (to refer to our own RPM directory tree)
    #
    def CreateRPMDirectiveFile(self):
        # Create the RPM directory tree

        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/BUILD"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/RPMS/athlon"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/RPMS/i386"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/RPMS/i486"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/RPMS/i586"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/RPMS/i686"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/RPMS/noarch"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/SOURCES"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/SPECS"))
        scxutil.MkAllDirs(os.path.join(self.targetDir, "RPM-packages/SRPMS"))

        # Create the RPM directive file

        if os.path.exists(os.path.join(os.path.expanduser('~'), '.rpmmacros')):
            scxutil.Move(os.path.join(os.path.expanduser('~'), '.rpmmacros'),
                         os.path.join(os.path.expanduser('~'), '.rpmmacros.save'))

        rpmfile = open(os.path.join(os.path.expanduser('~'), '.rpmmacros'), 'w')
        rpmfile.write('%%_topdir\t%s\n' % os.path.join(self.targetDir, "RPM-packages"))
        rpmfile.close()

    ##
    # Cleanup RPM directive file
    #
    # (If a prior version of the file exists, retain that)
    #
    def DeleteRPMDirectiveFile(self):
        if os.path.exists(os.path.join(os.path.expanduser('~'), '.rpmmacros.save')):
            scxutil.Move(os.path.join(os.path.expanduser('~'), '.rpmmacros.save'),
                         os.path.join(os.path.expanduser('~'), '.rpmmacros'))
        else:
            os.unlink(os.path.join(os.path.expanduser('~'), '.rpmmacros'))

    ##
    # Actually creates the finished package file.
    #
    def BuildPackage(self):
        # Create the RPM working directory tree
        self.CreateRPMDirectiveFile()

        # Build the RPM. This puts the finished rpm in /usr/src/packages/RPMS/<arch>/
        os.system('rpmbuild --buildroot ' + self.stagingDir.GetRootPath() + ' -bb ' + self.specFileName)
        self.DeleteRPMDirectiveFile()

        # Now we try to find the file so we can copy it to the installer directory.
        # We need to find the build arch of the file:
        fin, fout = os.popen4('rpm -q --specfile --qf "%{arch}\n" ' + self.specFileName)
        arch = fout.read().strip()
        rpmpath = os.path.join(os.path.join(self.targetDir, "RPM-packages/RPMS"), arch)
        if self.configuration['pfdistro'] == 'SUSE':
            rpmNewFileName = 'scx-' + \
                             self.configuration['version'] + '-' + \
                             self.configuration['release'] + '.sles.' + \
                             str(self.configuration['pfmajor']) + '.' + self.configuration['pfarch'] + '.rpm'
        elif self.configuration['pfdistro'] == 'REDHAT':
            rpmNewFileName = 'scx-' + \
                             self.configuration['version'] + '-' + \
                             self.configuration['release'] + '.rhel.' + \
                             str(self.configuration['pfmajor'])  + '.' + self.configuration['pfarch'] + '.rpm'
        else:
            raise PlatformNotImplementedError(self.configuration['pfdistro'])

        rpmfilename = self.configuration['short_name'] + '-' + \
                      self.configuration['version'] + '-' + \
                      self.configuration['release'] + '.' + arch + '.rpm'
                         
        
        scxutil.Move(os.path.join(rpmpath, rpmfilename), os.path.join(self.targetDir, rpmNewFileName))
        print "Moved to: " + os.path.join(self.targetDir, rpmNewFileName)

