#!/bin/bash

set -euo pipefail

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"

INSTALL_PATH=$SCRIPTS_PATH/../../install
QT_DIR_OPTION=""
CACKAGES_PATH=$SCRIPTS_PATH/../../packages
PACKAGES_PATH=$SCRIPTS_PATH/../../packages
CERT_ID=""
NOTAR_USER=""
NOTAR_TEAM_ID=""
NOTAR_PASSWORD=""

#checking for parameters
for i in "$@"
do
case $i in
    -ci=*|--cert_id=*)
        CERT_ID="${i#*=}"
        shift # past argument=value
    ;;
    -nu=*|--notarization_user=*)
        NOTAR_USER="${i#*=}"
        shift # past argument=value
                ;;  
    -np=*|--notarization_pssw=*)
        NOTAR_PASSWORD="${i#*=}"
        shift # past argument=value
        ;;
    -nt=*|--notarization_team=*)
        NOTAR_TEAM_ID="${i#*=}"
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

bash $SCRIPTS_PATH/internal/2a_appbundle.sh -i=$INSTALL_PATH $QT_DIR_OPTION

echo "======= AppBundle Created ======="

if [ -n "$CERT_ID" ] ; then
    bash $SCRIPTS_PATH/internal/2b_sign_appbundle.sh -i=$INSTALL_PATH -ci=$CERT_ID

    echo "======= AppBundle Signed ======="
fi

if [ -n "$NOTAR_USER" ] ; then
    bash $SCRIPTS_PATH/internal/2c_notarize_appbundle.sh -i=$INSTALL_PATH -nu=$NOTAR_USER -nt=$NOTAR_TEAM_ID -np=$NOTAR_PASSWORD

    echo "======= AppBundle Notarized ======="
fi

bash $SCRIPTS_PATH/internal/2d_dmg.sh -i=$INSTALL_PATH -p=$PACKAGES_PATH

echo "======= DMG Created ======="
