LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES = \
    src/hb-aat-layout.cc \
    src/hb-aat-map.cc \
    src/hb-blob.cc \
    src/hb-buffer-serialize.cc \
    src/hb-buffer-verify.cc \
    src/hb-buffer.cc \
    src/hb-common.cc \
    src/hb-draw.cc \
    src/hb-face.cc \
    src/hb-fallback-shape.cc \
    src/hb-font.cc \
    src/hb-ft.cc \
    src/hb-number.cc \
    src/hb-ot-cff1-table.cc \
    src/hb-ot-cff2-table.cc \
    src/hb-ot-color.cc \
    src/hb-ot-face.cc \
    src/hb-ot-font.cc \
    src/hb-ot-layout.cc \
    src/hb-ot-map.cc \
    src/hb-ot-math.cc \
    src/hb-ot-metrics.cc \
    src/hb-ot-shaper-arabic.cc \
    src/hb-ot-shaper-default.cc \
    src/hb-ot-shaper-hangul.cc \
    src/hb-ot-shaper-hebrew.cc \
    src/hb-ot-shaper-indic.cc \
    src/hb-ot-shaper-indic-table.cc \
    src/hb-ot-shaper-khmer.cc \
    src/hb-ot-shaper-myanmar.cc \
    src/hb-ot-shaper-syllabic.cc \
    src/hb-ot-shaper-thai.cc \
    src/hb-ot-shaper-use.cc \
    src/hb-ot-shaper-vowel-constraints.cc \
    src/hb-ot-shape.cc \
    src/hb-ot-shape-fallback.cc \
    src/hb-ot-shape-normalize.cc \
    src/hb-ot-tag.cc \
    src/hb-ot-var.cc \
    src/hb-outline.cc \
    src/hb-paint.cc \
    src/hb-paint-extents.cc \
    src/hb-set.cc \
    src/hb-shape-plan.cc \
    src/hb-shape.cc \
    src/hb-shaper.cc \
    src/hb-static.cc \
    src/hb-ucd.cc \
    src/hb-coretext.cc \
    src/hb-gdi.cc \
    src/hb-uniscribe.cc \
    src/hb-unicode.cc \


LOCAL_ARM_MODE := arm

LOCAL_CPP_EXTENSION := .cc

LOCAL_C_INCLUDES = \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/src/ \
    $(LOCAL_PATH)/../freetype/include/ \

LOCAL_STATIC_LIBRARIES += freetype

#LOCAL_CFLAGS += -DHB_NO_MT -DHAVE_OT -DHAVE_UCDN -fPIC
LOCAL_CFLAGS += -DHAVE_CONFIG_H -fPIC

LOCAL_EXPORT_C_INCLUDES = $(LOCAL_PATH)/src/

# -DHAVE_ICU -DHAVE_ICU_BUILTIN
LOCAL_MODULE:= harfbuzz

include $(BUILD_STATIC_LIBRARY)
