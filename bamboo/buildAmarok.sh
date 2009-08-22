#!/bin/bash
echo "Configuring Amarok"
#cmake ../checkout -DCMAKE_BUILD_TYPE=debugfull -DCMAKE_INSTALL_PREFIX=~/kde
echo "Building Amarok"
#make -j5
echo "Installing Amarok"
#make install
echo "Running tests"
#amarok --test --nofork
echo "Waiting for tests to finish..."
#sleep 5s
echo "Copying tests to Bamboo"
ant -f /home/atlassian/buildAmarok/build.xml qtest2junit
