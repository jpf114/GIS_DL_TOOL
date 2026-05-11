set(GIS_AI_RUNTIME_DLLS
    abseil_dll.dll
    aec.dll
    brotlicommon.dll
    brotlidec.dll
    brotlienc.dll
    bz2.dll
    charset-1.dll
    deflate.dll
    double-conversion.dll
    fontconfig-1.dll
    freetype.dll
    freexl-1.dll
    gdal.dll
    geos.dll
    geos_c.dll
    geotiff.dll
    gif.dll
    harfbuzz.dll
    harfbuzz-raster.dll
    harfbuzz-subset.dll
    harfbuzz-vector.dll
    hdf5.dll
    hdf5_hl.dll
    Iex-3_4.dll
    iconv-2.dll
    IlmThread-3_4.dll
    Imath-3_2.dll
    icudt78.dll
    icuin78.dll
    icuio78.dll
    icutu78.dll
    icuuc78.dll
    jpeg62.dll
    json-c.dll
    Lerc.dll
    libcrypto-3-x64.dll
    libcurl.dll
    libexpat.dll
    liblzma.dll
    libpng16.dll
    libpq.dll
    libprotobuf.dll
    libsharpyuv.dll
    libssl-3-x64.dll
    libwebp.dll
    libwebpdecoder.dll
    libwebpdemux.dll
    libwebpmux.dll
    libxml2.dll
    lz4.dll
    md4c.dll
    md4c-html.dll
    minizip.dll
    netcdf.dll
    onnxruntime.dll
    openjp2.dll
    OpenEXR-3_4.dll
    OpenEXRCore-3_4.dll
    pcre2-8.dll
    proj_9.dll
    qhull_r.dll
    spatialite.dll
    sqlite3.dll
    szip.dll
    tiff.dll
    tinyxml2.dll
    turbojpeg.dll
    uriparser.dll
    zlib1.dll
    zstd.dll
)

set(GIS_AI_RUNTIME_DLLS_DEBUG
    aecd.dll
    brotlicommond.dll
    brotlidecd.dll
    brotliencd.dll
    bz2d.dll
    charset-1d.dll
    deflated.dll
    double-conversiond.dll
    fontconfig-1d.dll
    freetyped.dll
    freexld.dll
    gdald.dll
    geos_cd.dll
    geosd.dll
    geotiff_d.dll
    gifd.dll
    harfbuzzd.dll
    harfbuzz-rasterd.dll
    harfbuzz-subsetd.dll
    harfbuzz-vectord.dll
    hdf5d.dll
    hdf5_hld.dll
    Iex-3_4_d.dll
    iconv-2d.dll
    IlmThread-3_4_d.dll
    Imath-3_2_d.dll
    icudtd78.dll
    icuind78.dll
    icuiod78.dll
    icutud78.dll
    icuucd78.dll
    jpeg62d.dll
    json-cd.dll
    Lercd.dll
    libcrypto-3-x64d.dll
    libcurl-d.dll
    libexpatd.dll
    liblzmad.dll
    libpng16d.dll
    libpqd.dll
    libprotobufd.dll
    libsharpyuvd.dll
    libssl-3-x64d.dll
    libwebpd.dll
    libwebpdecoderd.dll
    libwebpdemuxd.dll
    libwebpmuxd.dll
    libxml2d.dll
    lz4d.dll
    md4cd.dll
    md4c-htmld.dll
    minizipd.dll
    netcdfd.dll
    onnxruntimed.dll
    openjp2d.dll
    OpenEXR-3_4_d.dll
    OpenEXRCore-3_4_d.dll
    pcre2-8d.dll
    proj_9_d.dll
    qhull_rd.dll
    spatialited.dll
    sqlite3d.dll
    szipd.dll
    tiffd.dll
    tinyxml2d.dll
    turbojpegd.dll
    uriparserd.dll
    zlibd1.dll
    zstdd.dll
)

function(gis_ai_copy_minimal_runtime target_name)
    cmake_parse_arguments(ARG "" "DEST_DIR" "" ${ARGN})

    if(ARG_DEST_DIR)
        set(_dest "${ARG_DEST_DIR}")
    else()
        set(_dest "$<TARGET_FILE_DIR:${target_name}>")
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_dest}"
    )

    foreach(_dll IN LISTS GIS_AI_RUNTIME_DLLS)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${GIS_AI_VCPKG_RELEASE_BIN_DIR}/${_dll}"
                "${_dest}/${_dll}"
        )
    endforeach()

    foreach(_dll IN LISTS GIS_AI_RUNTIME_DLLS_DEBUG)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -D_SRC="${GIS_AI_VCPKG_DEBUG_BIN_DIR}/${_dll}"
                -D_DST="${_dest}/${_dll}"
                -P "${CMAKE_SOURCE_DIR}/cmake/copy_if_exists.cmake"
        )
    endforeach()
endfunction()
