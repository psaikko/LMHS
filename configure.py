#!/usr/bin/python3
import os
import subprocess
from subprocess import Popen

studio_dir = cplex_dir = cplex_lib_dir = concert_dir = concert_lib_dir = scip_dir = "-"

def path_of(dn, fn, t):
  find = Popen(["find",dn,"-type",t], stdout=subprocess.PIPE)
  grep = Popen(["grep",fn], stdin=find.stdout, stdout=subprocess.PIPE)
  filepath, _ = grep.communicate()  
  filepath = str(filepath, "ASCII").split('\n')[0]
  return os.path.dirname(filepath)

print("""This script generates a file 'config.mk' with IP solver library paths.
A path is required for only one of CPLEX/SCIP.""")
print("""
CPLEX Studio installation path (should contain 'cplex' and 'concert' directories)""")
while studio_dir == '-':
  studio_dir = input("> ").strip()
  if studio_dir == "": break
  if not os.path.isdir(studio_dir):
    print("%s is not a directory" % studio_dir)
    studio_dir = '-'
  else:
    find = Popen(["find",studio_dir,"-type","f"], stdout=subprocess.PIPE)
    grep = Popen(["grep", "libcplex.a"], stdin=find.stdout, stdout=subprocess.PIPE)
    cplex_lib_dir = path_of(studio_dir, "libcplex.a", 'f')
    cplex_dir = path_of(studio_dir, "cplex/include", 'd')
    concert_lib_dir = path_of(studio_dir, "libconcert.a", 'f')
    concert_dir = path_of(studio_dir, "concert/include", 'd')

print("""
SCIP path (Should contain SCIP's 'lib' and 'make' directories)""")
while scip_dir == '-':
  scip_dir = input("> ").strip()
  if scip_dir == "": break
  if not os.path.isdir(scip_dir):
    print("%s is not a directory" % scip_dir)
    scip_dir = '-'

with open('config.mk', 'w') as config_file:
  config_file.write("CPLEXDIR=%s\n""CPLEXLIBDIR=%s\nCONCERTDIR=%s\nCONCERTLIBDIR=%s" \
                    "\nSCIPDIR=%s\n" \
                    % (cplex_dir, cplex_lib_dir, concert_dir, concert_lib_dir, scip_dir))

if scip_dir == "" and studio_dir == "": print("Warning: no IP solver configured")
