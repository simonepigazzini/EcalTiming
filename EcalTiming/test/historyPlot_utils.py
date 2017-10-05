#!/usr/bin/env python

import sys, math, os, collections, getopt, subprocess, time, shutil, datetime
from ROOT import TFile, TH1, TH1F, TCanvas, TLegend, TGraph, TGraphErrors, gROOT, gPad, TAttText, TText, TGaxis, TMath, TStyle, TColor
from array import array

def computeMean (calib, calib_err) :
   mean = 0.
   den = 0.
   for pos,x in enumerate(calib):    
      if(float(calib_err[pos]) == 0.):
         continue
      mean = mean + float(calib[pos])/(float(calib_err[pos])*float(calib_err[pos])) 
      den = den + 1./(float(calib_err[pos])*float(calib_err[pos]))  
   mean = mean/den
   return mean    

def computeError (calib_err) :
   error = 0.
   for pos,x in enumerate(calib_err):  
      if(float(calib_err[pos]) == 0.):
         continue  
      error = error + 1./(float(calib_err[pos])*float(calib_err[pos]))  
   error = 1./math.sqrt(error)
   return error    

def calibFromXML (inFile, date, icount, timeStamp_list, allCalib_list, g_EBMinus, g_EBPlus, g_EEMinus, g_EEPlus) : 
   with open(inFile) as f_dump:
      data_dump = f_dump.read()
      lines_dump = data_dump.splitlines()  
      #fill with ICs
      for pos2,x2 in enumerate(lines_dump):
         x2 = x2.replace("<item>", "")
         x2 = x2.replace("</item>", "")
         if(pos2>7 and pos2<30608): 
            EB_minus.append(float(x2))
         if(pos2>30607 and pos2<61208): 
            EB_plus.append(float(x2))
         if(pos2>61213 and pos2<68539): 
            EE_minus.append(float(x2))
         if(pos2>68538 and pos2<75862): 
            EE_plus.append(float(x2)) 
   #fill the graph
   EB_p_mean = float(sum(EB_plus))/len(EB_plus)
   allCalib_list.append(float(EB_p_mean))
   EB_m_mean = float(sum(EB_minus))/len(EB_minus)
   allCalib_list.append(float(EB_m_mean))
   EE_p_mean = float(sum(EE_plus))/len(EE_plus)
   allCalib_list.append(float(EE_p_mean))
   EE_m_mean = float(sum(EE_minus))/len(EE_minus)
   allCalib_list.append(float(EE_m_mean))
   print "---> EB+: ",EB_p_mean,", EB-:",EB_m_mean,", EE+:",EE_p_mean,", EE-:",EE_m_mean,"\n"
   timeStamp = time.mktime(datetime.datetime.strptime(date, "%d/%m/%Y").timetuple())
   timeStamp_list.append(float(timeStamp))
   g_EBMinus.SetPoint(icount,timeStamp,EB_m_mean)
   g_EBPlus.SetPoint(icount,timeStamp,EB_p_mean)
   g_EEMinus.SetPoint(icount,timeStamp,EE_m_mean)
   g_EEPlus.SetPoint(icount,timeStamp,EE_p_mean) 

def calibFromDAT (inFile, date, icount, timeStamp_list, allCalib_list, g_EBMinus, g_EBPlus, g_EEMinus, g_EEPlus) : 
   if(inFile.find("#") != -1):
      return
   with open(inFile) as f_dump:
      data_dump = f_dump.read()
      lines_dump = data_dump.splitlines()  
      EB_plus = []
      EB_error_plus = []
      EB_minus = []
      EB_error_minus = []
      EE_plus = []
      EE_error_plus = []
      EE_minus = []
      EE_error_minus = []
      #fill with ICs
      for pos2,x2 in enumerate(lines_dump):
         calib = x2.split()
         if(calib[3] == "nan" or calib[3] == "-nan"):
            calib[3] = 0.
         if(float(calib[0])<0. and float(calib[2]) == 0.): 
            EB_minus.append(float(calib[3]))
            EB_error_minus.append(float(calib[4])/math.sqrt(int(calib[5])))
         if(float(calib[0])>0. and float(calib[2]) == 0.):
            EB_plus.append(float(calib[3]))
            EB_error_plus.append(float(calib[4])/math.sqrt(int(calib[5])))
         if(calib[2] == "-1"):
            EE_minus.append(float(calib[3]))
            EE_error_minus.append(float(calib[4])/math.sqrt(int(calib[5])))
         if(calib[2] == "1"):
            EE_plus.append(float(calib[3])) 
            EE_error_plus.append(float(calib[4])/math.sqrt(int(calib[5])))

   #fill the graph
   if(len(EB_plus) != 0):
      #EB_p_mean = computeMean(EB_plus,EB_error_plus)
      EB_p_mean = float(sum(EB_plus))/len(EB_plus)
      EB_p_error = computeError(EB_error_plus)
      allCalib_list.append(float(EB_p_mean))
   else:
      EB_p_mean = -999.
      EB_p_error = 0.
   if(len(EB_minus) != 0):
      #EB_m_mean = computeMean(EB_minus,EB_error_minus)
      EB_m_mean = float(sum(EB_minus))/len(EB_minus)
      EB_m_error = computeError(EB_error_minus)
      allCalib_list.append(float(EB_m_mean))
   else:
      EB_m_mean = -999.
      EB_m_error = 0.
   if(len(EE_plus) != 0):
      #EE_p_mean = computeMean(EE_plus,EE_error_plus)
      EE_p_mean = float(sum(EE_plus))/len(EE_plus)
      EE_p_error = computeError(EE_error_plus)
      allCalib_list.append(float(EE_p_mean))
   else:
      EE_p_mean = -999.
      EE_p_error = 0.
   if(len(EE_minus) != 0):
      #EE_m_mean = computeMean(EE_minus,EE_error_minus)
      EE_m_mean = float(sum(EE_minus))/len(EE_minus)
      EE_m_error = computeError(EE_error_minus)
      allCalib_list.append(float(EE_m_mean))
   else:
      EE_m_mean = -999.
      EE_m_error = 0.

   print date," ---> EB+: ",EB_p_mean,", EB-:",EB_m_mean,", EE+:",EE_p_mean,", EE-:",EE_m_mean,"\n"
   timeStamp = time.mktime(datetime.datetime.strptime(date, "%d/%m/%Y").timetuple())
   timeStamp_list.append(float(timeStamp))
   g_EBMinus.SetPoint(icount,timeStamp,EB_m_mean)
   #g_EBMinus.SetPointError(icount,0.,float(EB_m_error))
   g_EBPlus.SetPoint(icount,timeStamp,EB_p_mean)
   #g_EBPlus.SetPointError(icount,0.,float(EB_p_error))
   g_EEMinus.SetPoint(icount,timeStamp,EE_m_mean)
   #g_EEMinus.SetPointError(icount,0.,float(EE_m_error))
   g_EEPlus.SetPoint(icount,timeStamp,EE_p_mean)  
   #g_EEPlus.SetPointError(icount,0.,float(EE_p_error))

