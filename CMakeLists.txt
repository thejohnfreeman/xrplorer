cmake_minimum_required(VERSION 3.21)

project(xrplorer
  VERSION 0.1.0
  LANGUAGES CXX
)

find_package(cupcake.cmake REQUIRED)

cupcake_project()

cupcake_find_packages(main)
cupcake_link_libraries(${PROJECT_NAME}.imports.main INTERFACE main)
cupcake_find_packages(tool PRIVATE)

cupcake_add_libraries()

cupcake_add_executables()

cupcake_enable_testing()

cupcake_install_project()
cupcake_install_cpp_info()
