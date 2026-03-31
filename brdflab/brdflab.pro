QT += widgets xml concurrent charts
CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += _USE_MATH_DEFINES
DEFINES += NOMINMAX
DEFINES += HAVE_LCMS2

INCLUDEPATH += ../external/

win32:INCLUDEPATH += ../external/libjpeg-turbo-2.0.6/include \
    ../external/eigen-3.3.9/ \
    ../src/ \
    ../relightlab/
win32:LIBS += ../external/libjpeg-turbo-2.0.6/lib/jpeg-static.lib

unix::QMAKE_CXXFLAGS = -fopenmp
unix:INCLUDEPATH += ../external/eigen-3.3.9/ \
    ../src/ \
    ../relightlab/
#do not remove this variable.
ABSLLIBS = -Wl,--start-group -labsl_random_seed_sequences -labsl_status -labsl_flags_program_name -labsl_crc_cord_state -labsl_utf8_for_code_point -labsl_poison -labsl_random_internal_pool_urbg -labsl_demangle_rust -labsl_symbolize -labsl_civil_time -labsl_string_view -labsl_flags_parse -labsl_log_internal_conditions -labsl_strerror -labsl_strings_internal -labsl_cordz_info -labsl_stacktrace -labsl_scoped_set_env -labsl_kernel_timeout_internal -labsl_raw_logging_internal -labsl_die_if_null -labsl_log_internal_structured_proto -labsl_log_initialize -labsl_flags_private_handle_accessor -labsl_random_distributions -labsl_raw_hash_set -labsl_cordz_sample_token -labsl_log_sink -labsl_random_internal_platform -labsl_tracing_internal -labsl_log_entry -labsl_demangle_internal -labsl_cord -labsl_crc_cpu_detect -labsl_leak_check -labsl_exponential_biased -labsl_random_internal_seed_material -labsl_statusor -labsl_random_internal_distribution_test_util -labsl_flags_config -labsl_random_internal_randen_slow -labsl_flags_commandlineflag -labsl_log_severity -labsl_flags_usage_internal -labsl_examine_stack -labsl_flags_internal -labsl_spinlock_wait -labsl_flags_marshalling -labsl_crc32c -labsl_periodic_sampler -labsl_strings -labsl_low_level_hash -labsl_graphcycles_internal -labsl_log_flags -labsl_str_format_internal -labsl_hashtablez_sampler -labsl_hash -labsl_cordz_handle -labsl_flags_usage -labsl_random_internal_randen -labsl_log_globals -labsl_log_internal_log_sink_set -labsl_flags_reflection -labsl_bad_variant_access -labsl_log_internal_format -labsl_crc_internal -labsl_log_internal_check_op -labsl_random_internal_randen_hwaes -labsl_bad_optional_access -labsl_random_seed_gen_exception -labsl_vlog_config_internal -labsl_random_internal_randen_hwaes_impl -labsl_malloc_internal -labsl_int128 -labsl_log_internal_proto -labsl_cord_internal -labsl_bad_any_cast_impl -labsl_city -labsl_throw_delegate -labsl_decode_rust_punycode -labsl_debugging_internal -labsl_flags_commandlineflag_internal -labsl_log_internal_nullguard -labsl_log_internal_message -labsl_log_internal_fnmatch -labsl_cordz_functions -labsl_log_internal_globals -labsl_time_zone -labsl_base -labsl_failure_signal_handler -labsl_time -labsl_synchronization -Wl,--end-group
#ABSLLIBS =
unix:LIBS += -ljpeg -ltiff -llcms2 -lceres $$ABSLLIBS -lglog -lgflags -lspqr -lcholmod -lccolamd -lcamd -lcolamd -lamd -lmetis -lsuitesparseconfig -llapack -lblas
unix::LIBS += -fopenmp

mac:INCLUDEPATH += /usr/local/Cellar/jpeg-turbo/3.1.0/include \
    /usr/local/include \
    /usr/local/include/eigen3
mac:LIBS += -L/usr/local/Cellar/jpeg-turbo/3.1.0/lib/ -ljpeg -llcms2 -lceres -lglog -lgflags -lspqr -lcholmod -lccolamd -lcamd -lcolamd -lamd -lsuitesparseconfig
mac:LIBS += -framework Accelerate
mac:QMAKE_CXXFLAGS += -Xpreprocessor -I/usr/local/include
mac:LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib

win32:LCMS2_HOME = $$getenv(LCMS2_HOME)
win32:!isEmpty(LCMS2_HOME) {
    INCLUDEPATH += $$LCMS2_HOME/include
    LIBS += $$LCMS2_HOME/lib/lcms2.lib
} else {
    LIBS += -llcms2
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    ../src/align.cpp \
    ../src/brdf/brdf_math.cpp \
    ../src/brdf/brdf_optimizer.cpp \
    ../src/brdf/brdfparameters.cpp \
    ../src/brdf/brdftask.cpp \
    ../src/brdf/init_normals.cpp \
    ../src/colorprofile.cpp \
    ../src/crop.cpp \
    ../src/deepzoom.cpp \
    ../src/dome.cpp \
    ../src/exif.cpp \
    ../src/icc_profiles.cpp \
    ../src/image.cpp \
    ../src/imageset.cpp \
    ../src/jpeg_decoder.cpp \
    ../src/jpeg_encoder.cpp \
    ../src/legacy_rti.cpp \
    ../src/lens.cpp \
    ../src/lp.cpp \
	../src/measure.cpp \
	../src/network/httpserver.cpp \
	../src/project.cpp \
	../src/sphere.cpp \
	../src/task.cpp \
	../src/white.cpp \
	../relightlab/canvas.cpp \
    ../relightlab/imageview.cpp \
    diagnosticpanel.cpp \
    normalspherewidget.cpp \
    reflectanceview.cpp \
    rusinview.cpp

HEADERS += \
    mainwindow.h \
    ../src/brdf/brdf_ceres.h \
    ../src/brdf/brdf_math.h \
    ../src/brdf/brdf_optimizer.h \
    ../src/brdf/brdfparameters.h \
    ../src/brdf/brdftask.h \
    ../src/brdf/init_normals.h \
    ../src/task.h \
    ../relightlab/canvas.h \
    ../relightlab/imageview.h \
    diagnosticpanel.h \
    normalspherewidget.h \
    reflectanceview.h \
    rusinview.h

RESOURCES += \
    ../src/icc_profiles.qrc
