cmake_minimum_required(VERSION 3.7)

#
#	set_repository_gsl_dependency
#
function(set_repository_gsl_dependency)
	set_repository_library_dependency(
		NAME gsl
		DESCRIPTION "Microsoft Guidelines Support Library (GSL)"
		PACKAGE "libmsgsl-dev:libgsl-c++-dev"
		)
endfunction()

