target_sources(
  obs-studio
  PRIVATE
    importer/ImporterEntryPathItemDelegate.cpp
    importer/ImporterEntryPathItemDelegate.hpp
    importer/ImporterModel.cpp
    importer/ImporterModel.hpp
    importer/OBSImporter.cpp
    importer/OBSImporter.hpp
    importers/classic.cpp
    importers/importers.cpp
    importers/importers.hpp
    importers/sl.cpp
    importers/studio.cpp
    importers/xsplit.cpp
)

set_source_files_properties(importer/OBSImporter.cpp PROPERTIES COMPILE_FLAGS "-std=c++17")
