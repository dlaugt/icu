/*
 *****************************************************************************
 * Copyright (C) 2000-2002, International Business Machines Corporation and  *
 * others. All Rights Reserved.                                              *
 *****************************************************************************
 *
 * $Source: /xsrl/Nsvn/icu/unicodetools/com/ibm/rbm/RBPropertiesExporter.java,v $ 
 * $Date: 2004/06/29 18:45:43 $ 
 * $Revision: 1.2 $
 *
 *****************************************************************************
 */
package com.ibm.rbm;

import java.io.*;
import javax.swing.*;
import javax.swing.filechooser.*;
import java.util.*;

/**
 * This class provides a plug-in exporter utility for RBManager that outputs Java
 * standard .properties files in the according to the file structure of Resource
 * Bundles. Most all meta-data is lost in this export.
 * 
 * @author Jared Jackson - Email: <a href="mailto:jjared@almaden.ibm.com">jjared@almaden.ibm.com</a>
 * @see com.ibm.rbm.RBManager
 */
public class RBPropertiesExporter extends RBExporter {
	
    public RBPropertiesExporter() {
        super();
		
        // Initialize the file chooser if necessary
        if (chooser == null) {
            chooser = new JFileChooser();
            chooser.setFileFilter(new javax.swing.filechooser.FileFilter(){
                public String getDescription() {
                    return "Base Class Properties Files";
                }
                public boolean accept(File f) {
                    if (f.isDirectory()) return true;
                    String name = f.getName();
                    if (name.toLowerCase().endsWith(".properties") && f.getName().indexOf("_") < 0) return true;
                    return false;
                }
            });
        } // end if
    }
	
    public void export(RBManager rbm) throws IOException {
        if (rbm == null) return;
        // Open the Save Dialog
        int ret_val = chooser.showSaveDialog(null);
        if (ret_val != JFileChooser.APPROVE_OPTION) return;
        // Retrieve basic file information
        File file = chooser.getSelectedFile();                  // The file(s) we will be working with
        File directory = new File(file.getParent());            // The directory we will be writing to
        String base_name = file.getName();                      // The base name of the files we will write
        if (base_name == null || base_name.equals("")) base_name = rbm.getBaseClass();
        if (base_name.toLowerCase().endsWith(".properties")) 
            base_name = base_name.substring(0,base_name.length()-11);
		
        Vector bundle_v = rbm.getBundles();
        for (int i=0; i < bundle_v.size(); i++) {
            Properties prop = new Properties();
            Bundle bundle = (Bundle)bundle_v.elementAt(i);
            String base_enc = base_name;
            if (bundle.encoding != null && !bundle.encoding.equals("")) base_enc = base_enc + "_" + bundle.encoding;
            String file_name = base_enc + ".properties";
            String header = "Resource Bundle: " + file_name + " - File automatically generated by RBManager at " + (new Date());
			
            Vector group_v = bundle.getGroupsAsVector();
            for (int j=0; j < group_v.size(); j++) {
                BundleGroup group = (BundleGroup)group_v.elementAt(j);
                Vector item_v = group.getItemsAsVector();
                for (int k=0; k < item_v.size(); k++) {
                    BundleItem item = (BundleItem)item_v.elementAt(k);
                    prop.setProperty(item.getKey(), item.getTranslation());
                } // end for - k
            } // end for - j
			
            // Write out the file
            File write_file = new File(directory, file_name);
            FileOutputStream fos = new FileOutputStream(write_file);
            prop.store(fos, header);
        } // end for - i
    }
}