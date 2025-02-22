 #define COMPILING_MRISURF_TOPOLOGY_FRIEND_CHECKED
/**
 * @brief local utilities for GIFTI library
 *
 * This file has some some extra functions for use with the GIFTI
 * utilities. The official utilities reside in gifti_io.c and gifti_xml.c
 *
 */
/*
 * Original Authors: Kevin Teich and Nick Schmansky
 *
 * Copyright © 2021 The General Hospital Corporation (Boston, MA) "MGH"
 *
 * Terms and conditions for use, reproduction, distribution and contribution
 * are found in the 'FreeSurfer Software License Agreement' contained
 * in the file 'LICENSE' found in the FreeSurfer distribution, and here:
 *
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferSoftwareLicense
 *
 * Reporting: freesurfer@nmr.mgh.harvard.edu
 *
 */

#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "error.h"  // return codes
#include "gifti.h"
#include "nifti1.h"
#include "timer.h"
#include "utils.h"  // strcpyalloc

#define TAG_CMDLINE_LEN 1024

/*
 *
 */
static giiDataArray *gifti_alloc_and_add_darray(gifti_image *image)
{
  if (!image) {
    fprintf(stderr, "** gifti_alloc_and_add_darray: NULL image\n");
    return NULL;
  }

  /* Try to add an empty array. */
  if (gifti_add_empty_darray(image, 1)) {
    fprintf(stderr,
            "** gifti_alloc_and_add_darray: gifti_add_empty_darray "
            "failed\n");
    return NULL;
  }

  /* Return the array we just allocated. */
  return image->darray[image->numDA - 1];
}

/*
 *
 */
static double gifti_get_DA_value_2D(giiDataArray *da, int row, int col)
{
  int dim0_index, dim1_index;
  int dims_0 = 0, dims_1 = 0;

  if (!da || !da->data) {
    fprintf(stderr, "** gifti_get_DA_value_2D, invalid params: data=%p\n", da);
    exit(1);
  }

  if (da->num_dim == 1) {
    // support for using this routine to read 1D data, under one condition...
    if (col != 0) {
      fprintf(stderr,
              "** gifti_get_DA_value_2D, array dim is 1 "
              "but trying to access 2D data element (col=%d)\n",
              col);
      exit(1);
    }
    dims_0 = da->dims[0];
    dims_1 = 1;  // 1D data
  }
  else if (da->num_dim != 2) {
    fprintf(stderr, "** gifti_get_DA_value_2D, array dim is %d\n", da->num_dim);
    exit(1);
  }
  else {
    dims_0 = da->dims[0];
    dims_1 = da->dims[1];
  }

  /* Get the dim0 and dims[1] indices based on our order. */
  if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord) {
    dim0_index = row;
    dim1_index = col;
  }
  else if (GIFTI_IND_ORD_COL_MAJOR == da->ind_ord) {
    // NJS NOTE: notice that order is treated as row/col, so that the
    // calling sequence can just assume row major
    dim0_index = row;  // col;
    dim1_index = col;  // row;
  }
  else {
    fprintf(stderr, "** gifti_get_DA_value_2D, unknown ind_ord: %d\n", da->ind_ord);
    exit(1);
  }
  if (da->num_dim == 1)  // support for using this routine to read 1D data
  {
    dim0_index = row;
    dim1_index = col;
  }

  /* Check the indices. */
  if (dim0_index < 0 || dim0_index >= dims_0 || dim1_index < 0 || dim1_index >= dims_1) {
    fprintf(stderr,
            "** gifti_get_DA_value_2D, invalid params: "
            "dim0_index=%d (max=%d), dim1_index=%d (max=%d)\n",
            dim0_index,
            dims_0,
            dim1_index,
            dims_1);
    exit(1);
  }

  /* Switch on the data type and return the appropriate
     element. Indexing depends on the data order. */
  switch (da->datatype) {
    default:
      fprintf(stderr,
              "** gifti_get_DA_value_2D, unsupported type %d-"
              "unknown, or can't convert to double\n",
              da->datatype);
      exit(1);
    case NIFTI_TYPE_UINT8: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((unsigned char *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((unsigned char *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_INT16: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((short *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((short *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_INT32: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((int *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((int *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_FLOAT32: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((float *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((float *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_FLOAT64: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((double *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((double *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_COMPLEX64: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((double *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((double *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_INT8: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((char *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((char *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_UINT16: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((unsigned short *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((unsigned short *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
    case NIFTI_TYPE_UINT32: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        return (double)*((unsigned int *)(da->data) + (dim0_index * dims_1) + dim1_index);
      else
        return (double)*((unsigned int *)(da->data) + dim0_index + (dim1_index * dims_0));
      break;
    }
  }

  exit(1);
}

/*
 *
 */
static void gifti_set_DA_value_2D(giiDataArray *da, int row, int col, double value)
{
  int dim0_index, dim1_index;
  int dims_0 = 0, dims_1 = 0;

  if (!da || !da->data) {
    fprintf(stderr, "** gifti_set_DA_value_2D, invalid params: data=%p\n", da);
    exit(1);
  }

  if (da->num_dim == 1) {
    // support for using this routine to write 1D data, under one condition...
    if (col != 0) {
      fprintf(stderr,
              "** gifti_set_DA_value_2D, array dim is 1 "
              "but trying to access 2D data element (col=%d)\n",
              col);
      exit(1);
    }
    dims_0 = da->dims[0];
    dims_1 = 1;  // 1D data
  }
  else if (da->num_dim != 2) {
    fprintf(stderr, "** gifti_set_DA_value_2D, array dim is %d\n", da->num_dim);
    exit(1);
  }
  else {
    dims_0 = da->dims[0];
    dims_1 = da->dims[1];
  }

  /* Get the dim0 and dims[1] indices based on our order. */
  if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord) {
    dim0_index = row;
    dim1_index = col;
  }
  else {
    dim0_index = col;
    dim1_index = row;
  }
  if (da->num_dim == 1)  // support for using this routine to read 1D data
  {
    dim0_index = row;
    dim1_index = col;
  }

  /* Check the indices. */
  if (dim0_index < 0 || dim0_index >= dims_0 || dim1_index < 0 || dim1_index >= dims_1) {
    fprintf(stderr,
            "** gifti_set_DA_value_2D, invalid params: "
            "dim0_index=%d (max=%d), dim1_index=%d (max=%d)\n",
            dim0_index,
            dims_0,
            dim1_index,
            dims_1);
    return;
  }

  /* Switch on the data type and write the appropriate
     element. Indexing depends on the data order. */
  switch (da->datatype) {
    default:
      fprintf(stderr,
              "** gifti_set_DA_value_2D, unsupported type %d-"
              "unknown, or can't convert to double\n",
              da->datatype);
      return;
    case NIFTI_TYPE_UINT8: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        *((unsigned char *)(da->data) + (dim0_index * dims_1) + dim1_index) = (unsigned char)value;
      else
        *((unsigned char *)(da->data) + dim0_index + (dim1_index * dims_0)) = (unsigned char)value;
      break;
    }
    case NIFTI_TYPE_INT16: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        *((short *)(da->data) + (dim0_index * dims_1) + dim1_index) = (short)value;
      else
        *((short *)(da->data) + dim0_index + (dim1_index * dims_0)) = (short)value;
      break;
    }
    case NIFTI_TYPE_INT32: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        *((int *)(da->data) + (dim0_index * dims_1) + dim1_index) = (int)value;
      else
        *((int *)(da->data) + dim0_index + (dim1_index * dims_0)) = (int)value;
      break;
    }
    case NIFTI_TYPE_FLOAT32: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        *((float *)(da->data) + (dim0_index * dims_1) + dim1_index) = (float)value;
      else
        *((float *)(da->data) + dim0_index + (dim1_index * dims_0)) = (float)value;
      break;
    }
    case NIFTI_TYPE_INT8: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        *((char *)(da->data) + (dim0_index * dims_1) + dim1_index) = (char)value;
      else
        *((char *)(da->data) + dim0_index + (dim1_index * dims_0)) = (char)value;
      break;
    }
    case NIFTI_TYPE_UINT16: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        *((unsigned short *)(da->data) + (dim0_index * dims_1) + dim1_index) = (unsigned short)value;
      else
        *((unsigned short *)(da->data) + dim0_index + (dim1_index * dims_0)) = (unsigned short)value;
      break;
    }
    case NIFTI_TYPE_UINT32: {
      if (GIFTI_IND_ORD_ROW_MAJOR == da->ind_ord)
        *((unsigned int *)(da->data) + (dim0_index * dims_1) + dim1_index) = (unsigned int)value;
      else
        *((unsigned int *)(da->data) + dim0_index + (dim1_index * dims_0)) = (unsigned int)value;
      break;
    }
  }

  return;
}

/*-------------------------------------------------------------------
  Parameters:    input file name of GIFTI file
                 optional mris structure to store data found
                 optional data array number to read

  Returns value: freesurfer surface structure

  Description:   reads a GIFTI file, putting vertices,
                 and faces into an MRIS_SURFACE structure,
                 along with any other data, like labels,
                 colors, curv data, stats or values.
                 if daNum is not -1, then read only the
                 data in data array number daNum
  -------------------------------------------------------------------*/
MRIS *mrisReadGIFTIdanum(const char *fname, MRIS *mris, int daNum, MRI *outmri, int *frame)
{
  /*
   * attempt to read the file
   */
  gifti_image *image = gifti_read_image(fname, 1);
  if (NULL == image) {
    fprintf(stderr, "mrisReadGIFTIfile: gifti_read_image() returned NULL\n");
    return NULL;
  }

  // make sure version is recoded before validation
  if (!strcmp(image->version, "1")) {
    free(image->version);
    image->version = strcpyalloc(GIFTI_XML_VERSION);
  }

  /*
   * check for compliance
   */
  int valid = gifti_valid_gifti_image(image, 1);
  if (valid == 0) {
    fprintf(stderr, "mrisReadGIFTIfile: GIFTI file %s is invalid!\n", fname);
    gifti_free_image(image);
    return NULL;
  }

  /*
   * check for 'LabelTable' data and read into our colortable if exists
   */
  COLOR_TABLE *ct = NULL;
  if (image->labeltable.length > 0) {
    /* check validity of labeltable data */
    if (!gifti_valid_LabelTable(&image->labeltable, 1)) {
      fprintf(stderr, "mrisReadGIFTIfile: invalid labeltable found in file %s\n", fname);
      gifti_free_image(image);
      return NULL;
    }

    /* copy label table contents to our color_table struct */
    ct = (COLOR_TABLE *)calloc(1, sizeof(COLOR_TABLE));
    if (ct == NULL) {
      fprintf(stderr, "mrisReadGIFTIfile: could not alloc colortable memory\n");
      gifti_free_image(image);
      return NULL;
    }
    memset(ct, 0, sizeof(COLOR_TABLE));
    ct->nentries = image->labeltable.length;
    ct->version = 2;
    ct->entries = (COLOR_TABLE_ENTRY **)calloc(ct->nentries + 1, sizeof(COLOR_TABLE_ENTRY *));
    if (ct->entries == NULL) {
      fprintf(stderr, "mrisReadGIFTIfile: could not alloc colortable entries\n");
      gifti_free_image(image);
      return NULL;
    }
    // memset(ct->entries,0,sizeof(ct->entries)); // original
    memset(ct->entries, 0, sizeof(*ct->entries));  // changed by dng
    strncpy(ct->fname, fname, sizeof(ct->fname)-1);

    float *rgba = image->labeltable.rgba;
    if (NULL == rgba) {
      // optional rgba values are missing, so we must create colors for
      // the labels
      image->labeltable.rgba = (float *)calloc(image->labeltable.length, 4 * sizeof(float *));
      if (NULL == image->labeltable.rgba) {
        fprintf(stderr,
                "mrisReadGIFTIfile: "
                "couldn't allocate memory for labeltable.rgba\n");
        return NULL;
      }
      rgba = image->labeltable.rgba;
      setRandomSeed(12);  // so that color generation is consistent
      int label_index;
      for (label_index = 0; label_index < image->labeltable.length; label_index++) {
        rgba[0] = (float)randomNumber(0.0f, 1.0f);
        rgba[1] = (float)randomNumber(0.0f, 1.0f);
        rgba[2] = (float)randomNumber(0.0f, 1.0f);
        rgba[3] = 1.0f;
        rgba += 4;
      }
    }

    rgba = image->labeltable.rgba;
    int label_index;
    for (label_index = 0; label_index < image->labeltable.length; label_index++) {
      ct->entries[label_index] = (CTE *)malloc(sizeof(CTE));
      if (ct->entries[label_index] == NULL) {
        fprintf(stderr, "mrisReadGIFTIfile: could not alloc colortable entry\n");
        gifti_free_image(image);
        return NULL;
      }
      strncpy(
          ct->entries[label_index]->name,
	  image->labeltable.label[label_index],
	  sizeof(ct->entries[label_index]->name)-1);

      ct->entries[label_index]->rf = rgba[0];
      ct->entries[label_index]->ri = floor((rgba[0]) * 256);
      if (ct->entries[label_index]->ri > 255) {
        ct->entries[label_index]->ri = 255;
      }
      ct->entries[label_index]->gf = rgba[1];
      ct->entries[label_index]->gi = floor((rgba[1]) * 256);
      if (ct->entries[label_index]->gi > 255) {
        ct->entries[label_index]->gi = 255;
      }
      ct->entries[label_index]->bf = rgba[2];
      ct->entries[label_index]->bi = floor((rgba[2]) * 256);
      if (ct->entries[label_index]->bi > 255) {
        ct->entries[label_index]->bi = 255;
      }
      ct->entries[label_index]->af = rgba[3];
      ct->entries[label_index]->ai = floor((rgba[3]) * 256);
      if (ct->entries[label_index]->ai > 255) {
        ct->entries[label_index]->ai = 255;
      }
      rgba += 4;
      /*
        printf("RGBA: %d %d %d %d %f %f %f %f\n",
           ct->entries[label_index]->ri,
           ct->entries[label_index]->gi,
           ct->entries[label_index]->bi,
           ct->entries[label_index]->ai,
           ct->entries[label_index]->rf,
           ct->entries[label_index]->gf,
           ct->entries[label_index]->bf,
           ct->entries[label_index]->af);
      */
    }
    ct->entries[label_index] = NULL;
    CTABfindDuplicateNames(ct);

    // we're done, except that the colortable struct 'ct' will get stored
    // in the mris structure element 'ct' at the end of this routine, after
    // mris is known to exist
  }
  // end of LabelTable parsing (into colortable)

  /*
   * Now parse the DataArrays looking for coordinate and face data arrays,
   * so that we can create our mris structure
   */
  giiDataArray *coords = NULL;
  giiDataArray *faces = NULL;
  int numDA;
  for (numDA = 0; numDA < image->numDA; numDA++) {
    if (image->darray[numDA]->intent == NIFTI_INTENT_POINTSET) {
      coords = image->darray[numDA];
    }
    else if (image->darray[numDA]->intent == NIFTI_INTENT_TRIANGLE) {
      faces = image->darray[numDA];
    }
  }

  /*
   * if we found coordinate and face data....create mris struct and fill it
   */
  if (coords && faces) {
    /* Check the number of vertices and faces. */
    long long num_vertices = 0;
    long long num_cols = 0;
    if (coords->ind_ord == GIFTI_IND_ORD_ROW_MAJOR) {
      // RowMajorOrder
      gifti_DA_rows_cols(coords, &num_vertices, &num_cols);
    }
    else {
      // ColumnMajorOrder
      gifti_DA_rows_cols(coords, &num_cols, &num_vertices);
    }
    if (num_vertices <= 0 || num_cols != 3) {
      fprintf(stderr,
              "mrisReadGIFTIfile: malformed coords data array in file "
              "%s: num_vertices=%d num_cols=%d\n",
              fname,
              (int)num_vertices,
              (int)num_cols);
      gifti_free_image(image);
      return NULL;
    }
    long long num_faces = 0;
    num_cols = 0;
    if (faces->ind_ord == GIFTI_IND_ORD_ROW_MAJOR) {
      // RowMajorOrder
      gifti_DA_rows_cols(faces, &num_faces, &num_cols);
    }
    else {
      // ColumnMajorOrder
      gifti_DA_rows_cols(faces, &num_cols, &num_faces);
    }
    if (num_faces <= 0 || num_cols != 3) {
      fprintf(stderr,
              "mrisReadGIFTIfile: malformed faces data array in file "
              "%s: num_faces=%d num_cols=%d\n",
              fname,
              (int)num_faces,
              (int)num_cols);
      gifti_free_image(image);
      return NULL;
    }

    /* Try to allocate a surface. */
    mris = MRISalloc(num_vertices, num_faces);
    if (NULL == mris) {
      fprintf(stderr,
              "mrisReadGIFTIfile: failed to allocate an MRIS with "
              "%d vertices and %d faces\n",
              (int)num_vertices,
              (int)num_faces);
      gifti_free_image(image);
      return NULL;
    }

    /* Set some meta data in the mris. */
    strcpy(mris->fname, fname);
    mris->type = MRIS_TRIANGULAR_SURFACE;
    char *hemi = gifti_get_meta_value(&coords->meta, "AnatomicalStructurePrimary");
    if (hemi && (strcmp(hemi, "CortexRight") == 0)) {
      mris->hemisphere = RIGHT_HEMISPHERE;
    }
    else if (hemi && (strcmp(hemi, "CortexLeft") == 0)) {
      mris->hemisphere = LEFT_HEMISPHERE;
    }
    else {
      mris->hemisphere = NO_HEMISPHERE;
    }

    /* gifti uses real RAS by default */
    // This is not correct. As of 12/19/2022, the default surface XYZ is in tkregister space.
    // mris->useRealRAS will be set accordingly based on the value in <dataspace> tag later.
    mris->useRealRAS = 0;

    /* retrieve volume geometry info */
    {
      int vgvalid = 0;  // there are a total of 18 values
      char *stmp = gifti_get_meta_value(&coords->meta, "VolGeomWidth");
      if (stmp && (1 == sscanf(stmp, "%d", &mris->vg.width))) {
        vgvalid++;  // track valid volgeom values found
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomHeight");
      if (stmp && (1 == sscanf(stmp, "%d", &mris->vg.height))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomDepth");
      if (stmp && (1 == sscanf(stmp, "%d", &mris->vg.depth))) {
        vgvalid++;
      }

      stmp = gifti_get_meta_value(&coords->meta, "VolGeomXsize");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.xsize))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomYsize");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.ysize))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomZsize");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.zsize))) {
        vgvalid++;
      }

      stmp = gifti_get_meta_value(&coords->meta, "VolGeomX_R");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.x_r))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomX_A");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.x_a))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomX_S");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.x_s))) {
        vgvalid++;
      }

      stmp = gifti_get_meta_value(&coords->meta, "VolGeomY_R");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.y_r))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomY_A");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.y_a))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomY_S");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.y_s))) {
        vgvalid++;
      }

      stmp = gifti_get_meta_value(&coords->meta, "VolGeomZ_R");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.z_r))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomZ_A");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.z_a))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomZ_S");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.z_s))) {
        vgvalid++;
      }

      stmp = gifti_get_meta_value(&coords->meta, "VolGeomC_R");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.c_r))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomC_A");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.c_a))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "VolGeomC_S");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->vg.c_s))) {
        vgvalid++;
      }

      // we got all 18 values
      if (vgvalid == 18) {
        mris->vg.valid = 1;  // finally we can say its valid data
      }

      stmp = gifti_get_meta_value(&coords->meta, "SurfaceCenterX");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->xctr))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "SurfaceCenterY");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->yctr))) {
        vgvalid++;
      }
      stmp = gifti_get_meta_value(&coords->meta, "SurfaceCenterZ");
      if (stmp && (1 == sscanf(stmp, "%f", &mris->zctr))) {
        vgvalid++;
      }
    }

    /* retrieve TAG_GROUP_AVG_SURFACE_AREA info */
    {
      char *group_avg_surface_area = gifti_get_meta_value(&coords->meta, "TAG_GROUP_AVG_SURFACE_AREA");
      if (group_avg_surface_area)
        sscanf(group_avg_surface_area, "%f", &mris->group_avg_surface_area);
    }

    /* retrieve TAG_CMDLINE info */
    {
      char *ncmds = gifti_get_meta_value(&coords->meta, "NUM_TAG_CMDLINE");
      if (ncmds)
        sscanf(ncmds, "%d", &mris->ncmds);

      int numcmds = mris->ncmds;
      if (mris->ncmds > MAX_CMDS)
      {
        printf("[WARN] mrisReadGIFTIdanum():  too many commands (%d) in file. Only last %d will be saved!\n", mris->ncmds, MAX_CMDS);
        mris->ncmds = MAX_CMDS;
      }

      int toskip = (numcmds > MAX_CMDS) ? (numcmds - MAX_CMDS) : 0;
      while (toskip)
        gifti_get_meta_value(&coords->meta, "TAG_CMDLINE");

      for (int ncmd = 0; ncmd < mris->ncmds; ncmd++)
      {
        char tag[20] = {'\0'};
        sprintf(tag, "%s#%d", "TAG_CMDLINE", ncmd);

        char *cmdline = gifti_get_meta_value(&coords->meta, tag);
        if (!cmdline)
	{
          printf("[ERROR] TAG_CMDLINE out of sync\n");
          break;
	}

        mris->cmdlines[ncmd] = (char *)calloc(TAG_CMDLINE_LEN + 1, sizeof(char));
        if (mris->cmdlines[ncmd] == NULL)
            ErrorExit(ERROR_NOMEMORY, "mrisReadGIFTIdanum(): could not allocate %d byte cmdline", TAG_CMDLINE_LEN);
        //mris->cmdlines[ncmd][TAG_CMDLINE_LEN] = 0;
        memcpy(mris->cmdlines[ncmd], cmdline, TAG_CMDLINE_LEN);
      }
    }

    /* Copy in the vertices. */
    int vertex_index;
    for (vertex_index = 0; vertex_index < num_vertices; vertex_index++) {
      mris->vertices_topology[vertex_index].num = 0;
      float x = (float)gifti_get_DA_value_2D(coords, vertex_index, 0);
      float y = (float)gifti_get_DA_value_2D(coords, vertex_index, 1);
      float z = (float)gifti_get_DA_value_2D(coords, vertex_index, 2);
      MRISsetXYZ(mris,vertex_index,x,y,z);
      mris->vertices[vertex_index].origarea = -1;
    }
    mrisComputeSurfaceDimensions(mris);
    
    /* Copy in the faces. */
    int face_index;
    for (face_index = 0; face_index < num_faces; face_index++) {
      int face_vertex_index;
      for (face_vertex_index = 0; face_vertex_index < VERTICES_PER_FACE; face_vertex_index++) {
        vertex_index = (int)gifti_get_DA_value_2D(faces, face_index, face_vertex_index);
        mris->faces[face_index].v[face_vertex_index] = vertex_index;
        mris->vertices_topology[vertex_index].num++;
      }
    }
    // each vertex has a face list (faster than face list in some operations)
    for (vertex_index = 0; vertex_index < num_vertices; vertex_index++) {
      mris->vertices_topology[vertex_index].f = (int   *)calloc(mris->vertices_topology[vertex_index].num, sizeof(int));
      mris->vertices_topology[vertex_index].n = (uchar *)calloc(mris->vertices_topology[vertex_index].num, sizeof(uchar));
      mris->vertices_topology[vertex_index].num = 0;  // this gets re-calc'd next...
    }
    for (face_index = 0; face_index < mris->nfaces; face_index++) {
      FACE *face = &mris->faces[face_index];
      int n;
      for (n = 0; n < VERTICES_PER_FACE; n++)
        mris->vertices_topology[face->v[n]].f[mris->vertices_topology[face->v[n]].num++] =
            face_index;  // note that .num is auto-incremented!
    }
    for (vertex_index = 0; vertex_index < num_vertices; vertex_index++) {
      int n, m;
      for (n = 0; n < mris->vertices_topology[vertex_index].num; n++) {
        for (m = 0; m < VERTICES_PER_FACE; m++) {
          if (mris->faces[mris->vertices_topology[vertex_index].f[n]].v[m] == vertex_index) {
            mris->vertices_topology[vertex_index].n[n] = m;
          }
        }
      }
    }

    mrisCompleteTopology(mris);
    
    // check-for and read coordsys struct for talairach xform
    if (coords->coordsys && (coords->numCS > 0)) {
      int idx;
      for (idx = 0; idx < coords->numCS; idx++) {
        if (0 == strcmp(coords->coordsys[idx]->dataspace, "NIFTI_XFORM_SCANNER_ANAT")) {
          mris->useRealRAS = 1;
	}
        else if (0 == strcmp(coords->coordsys[idx]->dataspace, "NIFTI_XFORM_UNKNOWN")) {
          if (0 == strcmp(coords->coordsys[idx]->xformspace, "NIFTI_XFORM_TALAIRACH")) {
            int r, c;
            mris->SRASToTalSRAS_ = MatrixAlloc(4, 4, MATRIX_REAL);
            for (r = 1; r <= 4; r++) {
              for (c = 1; c <= 4; c++) {
                mris->SRASToTalSRAS_->rptr[r][c] = coords->coordsys[idx]->xform[r - 1][c - 1];
              }
            }
          }
        }
      }
    }

    /* other data structure essentials, namely:
     *  mrisComputeVertexDistances(mris);
     *  mrisReadTransform(mris, fname) ;
     *  mris->radius = MRISaverageRadius(mris) ;
     *  MRIScomputeMetricProperties(mris) ;
     *  MRISstoreCurrentPositions(mris) ;
     */
    MRIScomputeNormals(mris);
    UpdateMRIS(mris, fname);
  }
  // completed parsing of coordinate and face data

  // sanity-check, we ought to have an mris struct (either passed-in as a
  // parameter, or created when we found coord and face data)
  if (NULL == mris) {
    fprintf(stderr, "mriseadGIFTIfile: mris is NULL! found when parsing file %s\n", fname);
    gifti_free_image(image);
    return NULL;
  }

  /*
   * and dont forget to store the colortable (if one was found)
   */
  if (ct) {
    mris->ct = ct;
    // sanity-check
    int numEntries = 0;
    CTABgetNumberOfValidEntries(mris->ct, &numEntries);
    if (numEntries != image->labeltable.length) {
      fprintf(
          stderr, "mrisReadGIFTIfile: ct_entries:%d != labeltable_entries:%d\n", numEntries, image->labeltable.length);
      gifti_free_image(image);
      return NULL;
    }
  }

  /*
   * Now re-parse the DataArrays looking for all the other data type (except
   * coordinate and face data arrays) and fill-in mris structure as needed.
   */
  int found_curv_data = 0;          // track if multiple shape data arrays exist
  int found_statval_data = 0;       // track if multiple stat/val data arrays exist
  giiDataArray *node_index = NULL;  // support for sparse data storage
  long long num_index_nodes = 0;    // support for sparse data storage
  int startDAnum = 0;
  int endDAnum = image->numDA;
  if (daNum != -1)  // support for extracting one particular data array
  {
    startDAnum = daNum;
    endDAnum = daNum + 1;
  }
  for (numDA = startDAnum; numDA < endDAnum; numDA++) {
    giiDataArray *darray = image->darray[numDA];

    // did these already
    if ((darray->intent == NIFTI_INTENT_POINTSET) || (darray->intent == NIFTI_INTENT_TRIANGLE)) {
      continue;
    }

    /* support for sparse data storage.  this array contains a list of node
       numbers and it should be the first data array in the file. The remaining
       data arrays in the file that contain data assigned to nodes must contain
       the same number of elements as the NIFTI_INTENT_NODE_INDEX array. */
    if (darray->intent == NIFTI_INTENT_NODE_INDEX) {
      if (numDA != 0) {
        fprintf(stderr,
                "mrisReadGIFTIfile: NODE_INDEX data array found but its not the "
                "first data array in file %s\n",
                fname);
        gifti_free_image(image);
        return NULL;
      }
      long long num_cols = 0;

      if (darray->ind_ord == GIFTI_IND_ORD_ROW_MAJOR) {
        gifti_DA_rows_cols(darray, &num_index_nodes, &num_cols);
      }
      else {
        gifti_DA_rows_cols(darray, &num_cols, &num_index_nodes);
      }

      if (num_index_nodes <= 0 || num_index_nodes > mris->nvertices || num_cols > 1) {
        fprintf(stderr,
                "mrisReadGIFTIfile: malformed NODE_INDEX data array in file %s: "
                "num_index_nodes=%d num_cols=%d max nvertices=%d, num_cols>1\n",
                fname,
                (int)num_index_nodes,
                (int)num_cols,
                mris->nvertices);
        gifti_free_image(image);
        return NULL;
      }
      // else good to do, so store this node index info
      node_index = darray;
      continue;
    }
    else {
      /* Check the number of vertices, so we dont trounce the mris struct */
      long long num_vertices = 0;
      long long num_cols = 0;
      long long expected_num_cols = 1;
      if (darray->intent == NIFTI_INTENT_VECTOR) {
        expected_num_cols = 3;
      }
      else if (darray->intent == NIFTI_INTENT_RGB_VECTOR) {
        expected_num_cols = 3;
      }
      else if (darray->intent == NIFTI_INTENT_RGBA_VECTOR) {
        expected_num_cols = 4;
      }
      else if (darray->intent == NIFTI_INTENT_GENMATRIX) {
        expected_num_cols = 9;
      }

      if (darray->ind_ord == GIFTI_IND_ORD_ROW_MAJOR) {
        gifti_DA_rows_cols(darray, &num_vertices, &num_cols);
      }
      else {
        gifti_DA_rows_cols(darray, &num_cols, &num_vertices);
      }

      if (num_vertices <= 0 || num_vertices != mris->nvertices || num_cols > expected_num_cols) {
        fprintf(stderr,
                "mrisReadGIFTIfile: malformed data array [%d] in file %s: "
                "num_vertices=%d num_cols=%d expected nvertices=%d, num_cols=%d\n",
                numDA,
                fname,
                (int)num_vertices,
                (int)num_cols,
                mris->nvertices,
                (int)expected_num_cols);
        gifti_free_image(image);
        return NULL;
      }
    }

    /*
     * parse each intent type
     */
    if (darray->intent == NIFTI_INTENT_SHAPE) {
      // 'shape' data goes in our 'curv' data element of mris
      if (found_curv_data) {
        fprintf(stderr,
                "WARNING: a prior data array of shape data has already "
                "been read!  Skipping data in array #%d in file %s\n",
                numDA,
                fname);
      }
      else {
        found_curv_data++;

        if (*frame >= 1)
          fprintf(stderr, "WARNING: Skip saving %s data in array #%d (%s) in MRI \n",
                          gifti_intent_to_string(darray->intent), numDA, fname);

        if (node_index)  // sparse data storage
        {
          int nindex;
          for (nindex = 0; nindex < num_index_nodes; nindex++) {
            int vno = gifti_get_DA_value_2D(node_index, nindex, 0);
            if (mris->vertices[vno].ripflag) {
              continue;
            }
            mris->vertices[vno].curv = (float)gifti_get_DA_value_2D(darray, nindex, 0);
            if (outmri != NULL)
              MRIsetVoxVal(outmri, vno, 0, 0, *frame, mris->vertices[vno].curv);
          }
        }
        else  // regular indexing
        {
          int vno;
          for (vno = 0; vno < mris->nvertices; vno++) {
            if (mris->vertices[vno].ripflag) {
              continue;
            }
            mris->vertices[vno].curv = (float)gifti_get_DA_value_2D(darray, vno, 0);
            if (outmri != NULL)
              MRIsetVoxVal(outmri, vno, 0, 0, *frame, mris->vertices[vno].curv);
          }
        }

        if (outmri != NULL)
          (*frame)++;
      }
    } // NIFTI_INTENT_SHAPE
    else if (darray->intent == NIFTI_INTENT_LABEL) {
      // 'label' data goes into the 'annotation' data element of mris
      if ((NULL == mris->ct) || (NULL == ct))  // sanity-check
      {
        fprintf(stderr, "mrisReadGIFTIfile: NULL colortable\n");
        gifti_free_image(image);
        return NULL;
      }
      unsigned int *label_data = (unsigned int *)darray->data;
      int nindex = 0;    // index into node_index (if sparse data storage is used)
      int da_index = 0;  // index into the data array at hand
      int vno = 0;       // index into the mris struct (vertex number)
      while (vno < mris->nvertices) {
        if (node_index)  // sparse data storage support
        {
          vno = gifti_get_DA_value_2D(node_index, nindex, 0);
          da_index = nindex;
        }
        else  // regular indexing
        {
          da_index = vno;
        }

        if (mris->vertices[vno].ripflag) {
          continue;
        }
        int table_key = *(label_data + da_index);
        int table_index = 0;
        for (table_index = 0; table_index < ct->nentries; table_index++) {
          if (table_key == image->labeltable.key[table_index]) {
            // found the label key for this node
            break;
          }
        }
        int annotation = 0;  // default to no label found
        if ((table_index < ct->nentries) && (table_index >= 0)) {
          // printf("vno: %d, tidx: %d, name: %s\n",
          //     vno,table_index,ct->entries[table_index]->name);
          annotation = CTABrgb2Annotation(
              ct->entries[table_index]->ri, ct->entries[table_index]->gi, ct->entries[table_index]->bi);
        }
        mris->vertices[vno].annotation = annotation;

        // cross-check:
        int index = -1;
        int result = CTABfindAnnotation(mris->ct, mris->vertices[vno].annotation, &index);
        if ((result != NO_ERROR) || (index < 0) || (index > image->labeltable.length)) {
          fprintf(stderr,
                  "mrisReadGIFTIfile: label node data not found in colortable! "
                  "vno: %d, annot: %8.8X\n",
                  vno,
                  mris->vertices[vno].annotation);
          gifti_free_image(image);
          return NULL;
        }

        if (node_index)  // sparse data storage support
        {
          if (++nindex >= num_index_nodes) {
            break;
          }
        }
        else  // regular indexing
        {
          vno++;
        }
      }
    } // NIFTI_INTENT_LABEL
    else if (darray->intent == NIFTI_INTENT_VECTOR) {
      // 'vector' data goes in our 'dx,dy,dz' data element of mris
      int vno;
      for (vno = 0; vno < mris->nvertices; vno++) {
        if (mris->vertices[vno].ripflag) {
          continue;
        }
        mris->vertices[vno].dx = (float)gifti_get_DA_value_2D(darray, vno, 0);
        mris->vertices[vno].dy = (float)gifti_get_DA_value_2D(darray, vno, 1);
        mris->vertices[vno].dz = (float)gifti_get_DA_value_2D(darray, vno, 2);
      }
    } // NIFTI_INTENT_VECTOR
    else if ((darray->intent == NIFTI_INTENT_RGB_VECTOR) || (darray->intent == NIFTI_INTENT_RGBA_VECTOR)) {
      // 'rgba' data goes in our 'annotation' data element of mris
      int vno;
      for (vno = 0; vno < mris->nvertices; vno++) {
        if (mris->vertices[vno].ripflag) {
          continue;
        }
        int r, g, b;

        float red = (float)gifti_get_DA_value_2D(darray, vno, 0);
        float green = (float)gifti_get_DA_value_2D(darray, vno, 0);
        float blue = (float)gifti_get_DA_value_2D(darray, vno, 0);

        if (red > 1) {
          r = (int)red;
        }
        else {
          r = (int)floor(red * 256);
        }
        if (r > 255) {
          r = 255;
        }
        if (green > 1) {
          g = (int)green;
        }
        else {
          g = (int)floor(green * 256);
        }
        if (g > 255) {
          g = 255;
        }
        if (blue > 1) {
          b = (int)blue;
        }
        else {
          b = (int)floor(blue * 256);
        }
        if (b > 255) {
          b = 255;
        }

        MRISRGBToAnnot(r, g, b, mris->vertices[vno].annotation);
      }
    } // NIFTI_INTENT_RGB_VECTOR || NIFTI_INTENT_RGBA_VECTOR
    else if (darray->intent == NIFTI_INTENT_GENMATRIX) {
      fprintf(stderr,
              "WARNING: ignoring unsupported data array NIFTI_INTENT_GENMATRIX"
              " in file %s\n",
              fname);
    } // NIFTI_INTENT_GENMATRIX
    else {
      // 'statistics' and all other kinds of data we'll put in both our
      // 'stat' and 'val' data elements of the mris structure
      if (found_statval_data) {
        fprintf(stderr,
                "WARNING: a prior data array of stat/val data has already "
                "been read!  Skipping data in array #%d in file %s\n",
                numDA,
                fname);
      }
      else {
        found_statval_data++;

        if (*frame >= 1)
          fprintf(stderr, "WARNING: Skip saving %s data in array #%d (%s) in MRI \n",
                          gifti_intent_to_string(darray->intent), numDA, fname);

        if (node_index)  // sparse data storage
        {
          int nindex;
          for (nindex = 0; nindex < num_index_nodes; nindex++) {
            int vno = gifti_get_DA_value_2D(node_index, nindex, 0);
            if (mris->vertices[vno].ripflag) {
              continue;
            }
            mris->vertices[vno].val = (float)gifti_get_DA_value_2D(darray, nindex, 0);
            mris->vertices[vno].stat = (float)gifti_get_DA_value_2D(darray, nindex, 0);
            if (outmri != NULL)
              MRIsetVoxVal(outmri, vno, 0, 0, *frame, mris->vertices[vno].stat);
          }
        }
        else  // regular indexing
        {
          int vno;
          for (vno = 0; vno < mris->nvertices; vno++) {
            if (mris->vertices[vno].ripflag) {
              continue;
            }
            mris->vertices[vno].val = (float)gifti_get_DA_value_2D(darray, vno, 0);
            mris->vertices[vno].stat = (float)gifti_get_DA_value_2D(darray, vno, 0);
            if (outmri != NULL)
              MRIsetVoxVal(outmri, vno, 0, *frame, 0, mris->vertices[vno].stat);
          }
        }

        if (outmri != NULL)
          (*frame)++;
      }
    } // 'statistics' and all other kinds of data
  } // for each DAnum

  /*
   * And we're done.
   */
  gifti_free_image(image);

  return mris;
} // end of mrisReadGIFTIdanum()


 /*-----------------------------------------------------------
   mrisReadGIFTIfile()
                 reads a GIFTI file, putting vertices,
                 and faces into an MRIS_SURFACE structure,
                 along with any other data, like labels,
                 colors, curv data, stats or values.
                
   after read, 
     first SHAPE is saved in mris->curv; 
     first <STATS> is saved in mris->val and mris->stat;
     all SHAPE and <STATS> data arrays are saved as multi-frame MRI
  -----------------------------------------------------------*/
MRI_SURFACE *mrisReadGIFTIfile(const char *fname, MRI_SURFACE *mris, MRI *outmri, int *frame)
{
  // default read routine (read all data arrays)
  return mrisReadGIFTIdanum(fname, mris, -1, outmri, frame);
}

/*-----------------------------------------------------------
  MRISreadGiftiAsMRI() - reads GIFTI functional frames into
  an MRI volume struct, which is a retro-fit usage to store
  multiple frames of data (where in this case, a frame is one
  complete vector of vertices).
  This routine will only read NIFTI_INTENT_TIME_SERIES data
  arrays.
  -----------------------------------------------------------*/
MRI *MRISreadGiftiAsMRI(const char *fname, int read_volume)
{
  /* Attempt to read the file. */
  gifti_image *image = gifti_read_image(fname, 1);
  if (NULL == image) {
    fprintf(stderr, "MRISreadGiftiAsMRI: gifti_read_image() returned NULL\n");
    return NULL;
  }

  // make sure version is recoded before validation
  if (!strcmp(image->version, "1")) {
    free(image->version);
    image->version = strcpyalloc(GIFTI_XML_VERSION);
  }

  /* check for compliance */
  int valid = gifti_valid_gifti_image(image, 1);
  if (valid == 0) {
    fprintf(stderr, "MRISreadGiftiAsMRI: GIFTI file %s is invalid!\n", fname);
    gifti_free_image(image);
    return NULL;
  }

  /* check for overlay data */
  giiDataArray *scalars = NULL;
  int frame_count = 0;
  long long num_vertices = -1;
  long long num_cols = 0;
#define INTENT_CODE_MAX_IDX 4
  int intent_code[INTENT_CODE_MAX_IDX] = {
      NIFTI_INTENT_TIME_SERIES, NIFTI_INTENT_SHAPE, NIFTI_INTENT_NONE, NIFTI_INTENT_NORMAL};
  int intent_code_idx = 0;
  // search all DAs for time series, then shape, then none, then normal.
  // if time series found, check all DAs to make sure all the same size.
  for (intent_code_idx = 0; intent_code_idx < INTENT_CODE_MAX_IDX; intent_code_idx++) {
    int da_num = 0;
    do {
      scalars = gifti_find_DA(image, intent_code[intent_code_idx], da_num);
      if (NULL == scalars) {
        if (++da_num >= image->numDA) {
          break;
        }
        else {
          continue;
        }
      }
      frame_count++;
      long long nvertices = 0, ncols = 0;

      if (scalars->ind_ord == GIFTI_IND_ORD_ROW_MAJOR) {
        gifti_DA_rows_cols(scalars, &nvertices, &ncols);
      }
      else {
        gifti_DA_rows_cols(scalars, &ncols, &nvertices);
      }

      if (num_vertices == -1) {
        num_vertices = nvertices;
        num_cols = ncols;
      }
      else {
        if (num_vertices <= 0 || num_vertices != nvertices || ncols != 1) {
          fprintf(stderr,
                  "MRISreadGiftiAsMRI: malformed time-series data array in file "
                  "%s: nvertices=%d ncols=%d expected num_vertices=%d\n",
                  fname,
                  (int)nvertices,
                  (int)num_cols,
                  (int)num_vertices);
          gifti_free_image(image);
          return NULL;
        }
      }
      if (++da_num >= image->numDA) {
        break;
      }
      if ((intent_code[intent_code_idx] != NIFTI_INTENT_TIME_SERIES) &&
          (intent_code[intent_code_idx] != NIFTI_INTENT_SHAPE) && (intent_code[intent_code_idx] != NIFTI_INTENT_NONE) &&
          (intent_code[intent_code_idx] != NIFTI_INTENT_NORMAL)) {
        break;
      }
    } while (scalars);

    if (scalars) {
      break;  // found some data, no need to check other intents
    }
  }

  if (frame_count == 0) {
    fprintf(stderr, "MRISreadGiftiAsMRI: no overlay data found in file %s\n", fname);
    gifti_free_image(image);
    return NULL;
  }

  /* if we don't need to read the volume, just return a header */
  MRI *mri;
  if (!read_volume) {
    mri = MRIallocHeader(num_vertices, 1, 1, MRI_FLOAT, frame_count);
    mri->nframes = frame_count;
    // not sure this is the best way to do this (dng, 4/4/17)
    if (image->numDA > 0) {
      char *stmp = gifti_get_meta_value(&image->darray[0]->meta, "TimeStep");
      if (stmp) sscanf(stmp, "%f", &mri->tr);
    }
    return (mri);
  }

  /* Copy in each scalar frame to 'volume' frame. */
  mri = MRIallocSequence(num_vertices, 1, 1, MRI_FLOAT, frame_count);
  frame_count = 0;
  int da_num;
  // ??? intent_code_idx = 4 here ???
  // ??? need outer loop for through INTENT_CODE_MAX_IDX ???
  for (da_num = 0; da_num < image->numDA; da_num++) {
    scalars = gifti_find_DA(image, intent_code[intent_code_idx], da_num);
    if (NULL == scalars) {
      continue;
    }
    int vno;
    for (vno = 0; vno < num_vertices; vno++) {
      float val = (float)gifti_get_DA_value_2D(scalars, vno, 0);
      MRIsetVoxVal(mri, vno, 0, 0, frame_count, val);
    }
    // printf("frame #%d\n",frame_count);
    frame_count++;
  }

  // not sure this is the best way to do this (dng, 4/4/17)
  if (image->numDA > 0) {
    char *stmp = gifti_get_meta_value(&image->darray[0]->meta, "TimeStep");
    if (stmp) sscanf(stmp, "%f", &mri->tr);
  }

  /* And we're done. */
  gifti_free_image(image);
  return (mri);
} // end of MRISreadGiftiAsMRI()

/*
 * insert username and current date into meta data
 */
static void insertCommonMetaData(giiMetaData *md)
{
  struct passwd *pw = getpwuid(geteuid());
  if ((pw != NULL) && (pw->pw_name != NULL)) {
    gifti_add_to_meta(md, "UserName", pw->pw_name, 1);
  }

  gifti_add_to_meta(md, "Date", currentDateTime().c_str(), 1);
}

/*-------------------------------------------------------------------------
  Parameters:    mris - MRIS_SURFACE structure
                 intent_code - NIFTI_INTENT_* indicating
                   type of data in mris to write
                 out_fname - output file name of GIFTI file
                 curv_fname - if intent code is NIFTI_INTENT_SHAPE,
                   then this file contains the data to load into .curv field
                   prior to writing shape data to gifti output

  Returns value: 0 if passed, else error code

  Description:   writes a GIFTI file, if intent code is...
                 NIFTI_INTENT_POINTSET or _TRIANGLE, then write vertices and
                 face data
                 if NIFTI_INTENT_LABEL, then write LabelTable and annotations
                 if NIFTI_INTENT_<statistic>, then write .stats data
  ------------------------------------------------------------------------*/
int MRISwriteGIFTI(MRIS *mris, int intent_code, const char *out_fname, const char *curv_fname)
{
  if (NULL == mris || NULL == out_fname) {
    fprintf(stderr, "MRISwriteGIFTI: invalid parameter\n");
    return ERROR_BADPARM;
  }

  if (intent_code == NIFTI_INTENT_SHAPE && NULL == curv_fname) {
    fprintf(stderr, "MRISwriteGIFTI: invalid parameter: curv_fname is NULL\n");
    return ERROR_BADPARM;
  }

  gifti_image *image = (gifti_image *)calloc(1, sizeof(gifti_image));
  if (NULL == image) {
    fprintf(stderr, "MRISwriteGIFTI: couldn't allocate image\n");
    return ERROR_NOMEMORY;
  }
  image->version = strcpyalloc(GIFTI_XML_VERSION);

  insertCommonMetaData(&image->meta);
  if (strlen(mris->subject_name)) {
    gifti_add_to_meta(&image->meta, "SubjectID", mris->subject_name, 1);
  }

  int error = MRISwriteGIFTIIntent(mris, intent_code, image, out_fname, curv_fname);
  if (error != NO_ERROR)
      return error;

  // make sure version is recoded before validation
  if (!strcmp(image->version, "1")) {
    free(image->version);
    image->version = strcpyalloc(GIFTI_XML_VERSION);
  }

  /* check for compliance */
  int valid = gifti_valid_gifti_image(image, 1);
  if (valid == 0) {
    fprintf(stderr, "MRISwriteGIFTI: GIFTI file %s is invalid!\n", out_fname);
    gifti_free_image(image);
    return ERROR_BADFILE;
  }

  /* Write the file. */
  if (gifti_write_image(image, out_fname, 1)) {
    fprintf(stderr, "MRISwriteGIFTI: couldn't write image\n");
    gifti_free_image(image);
    return ERROR_BADFILE;
  }

  gifti_free_image(image);

  return ERROR_NONE;
} // end of MRISwriteGIFTI()

/*-----------------------------------------------------------
  Parameters:    MRI structure (surface-encoded volume),
                 output file name of GIFTI file,

  Returns value: 0 if passed, else error code

  Description:   writes a GIFTI file containing functional or
                 timeseries data
  -----------------------------------------------------------*/
int mriWriteGifti(MRI *mri, const char *out_fname)
{
  if (NULL == mri || NULL == out_fname) {
    fprintf(stderr, "mriWriteGifti: invalid input parameters\n");
    return ERROR_BADPARM;
  }

  gifti_image *image = (gifti_image *)calloc(1, sizeof(gifti_image));
  if (NULL == image) {
    fprintf(stderr, "mriWriteGifti: couldn't allocate gifti_image\n");
    return ERROR_NOMEMORY;
  }
  image->version = strcpyalloc(GIFTI_XML_VERSION);

  /* include some metadata describing this thing */
  insertCommonMetaData(&image->meta);
  /* -------------------------------------------------------
   * One DataArray for each 'frame' in the 'volume' data
   */
  int frame;
  for (frame = 0; frame < mri->nframes; frame++) {
    giiDataArray *scalars = gifti_alloc_and_add_darray(image);
    if (NULL == scalars) {
      fprintf(stderr, "mriWriteGifti: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    scalars->intent = NIFTI_INTENT_NONE;
    if (mri->nframes > 1) {
      scalars->intent = NIFTI_INTENT_TIME_SERIES;
    }
    if (scalars->intent == NIFTI_INTENT_TIME_SERIES) {
      // add TR (repetition time) to metadata:
      char buf[STRLEN];
      sprintf(buf, "%f", mri->tr);
      gifti_add_to_meta(&scalars->meta, "TimeStep", buf, 1);
    }
    scalars->datatype = NIFTI_TYPE_FLOAT32;
    scalars->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    scalars->num_dim = 1;
    scalars->dims[0] = mri->width;
    scalars->dims[1] = 0;
    scalars->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    scalars->endian = GIFTI_ENDIAN_LITTLE;
#else
    scalars->endian = GIFTI_ENDIAN_BIG;
#endif
    scalars->coordsys = NULL;
    scalars->nvals = gifti_darray_nvals(scalars);
    gifti_datatype_sizes(scalars->datatype, &scalars->nbyper, NULL);

    /* Allocate the data array. */
    scalars->data = NULL;
    scalars->data = (void *)calloc(scalars->nvals, scalars->nbyper);
    if (NULL == scalars->data) {
      fprintf(stderr,
              "mriWriteGifti: couldn't allocate scalars data of "
              "length %d, element size %d\n",
              (int)scalars->nvals,
              scalars->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our data. */
    int scalar_index;
    for (scalar_index = 0; scalar_index < mri->width; scalar_index++) {
      float val = MRIgetVoxVal(mri, scalar_index, 0, 0, frame);
      gifti_set_DA_value_2D(scalars, scalar_index, 0, val);
    }

    // next frame
  }

  // make sure version is recoded before validation
  if (!strcmp(image->version, "1")) {
    free(image->version);
    image->version = strcpyalloc(GIFTI_XML_VERSION);
  }

  /* check for compliance */
  int valid = gifti_valid_gifti_image(image, 1);
  if (valid == 0) {
    fprintf(stderr, "mriWriteGifti: GIFTI file %s is invalid!\n", out_fname);
    gifti_free_image(image);
    return ERROR_BADFILE;
  }

  /* Write the file. */
  if (gifti_write_image(image, out_fname, 1)) {
    fprintf(stderr, "mriWriteGifti: couldn't write image\n");
    gifti_free_image(image);
    return ERROR_BADFILE;
  }

  gifti_free_image(image);

  return ERROR_NONE;
} // end of mriWriteGifti()



// write single intent
int MRISwriteGIFTIIntent(MRIS *mris, int intent_code, gifti_image *image, const char *out_fname, const char *curv_fname)
{
  /* -------------------------------------------------------
   * Surface file
   */
  if (intent_code == NIFTI_INTENT_POINTSET || intent_code == NIFTI_INTENT_TRIANGLE) {
    int error = MRISwriteGIFTISurface(mris, image, out_fname); 
    if (error != NO_ERROR)
      return error;  
  }  // end of if NIFTI_INTENT_POINTSET or NIFTI_INTENT_TRIANGLE

  /* -------------------------------------------------------
   * Shape file
   */
  if (intent_code == NIFTI_INTENT_SHAPE) {
    int error = MRISwriteGIFTIShape(mris, image, intent_code, curv_fname);
    if (error != NO_ERROR)
      return error;
  }  // end of if NIFTI_INTENT_SHAPE

  /* -------------------------------------------------------
   * Label file
   */
  if (intent_code == NIFTI_INTENT_LABEL) {
    int error = MRISwriteGIFTILabel(mris, image, intent_code); 
    if (error != NO_ERROR)
      return error;    
  }  // end of if NIFTI_INTENT_LABEL

  /* -------------------------------------------------------
   * Statistics file
   */
  if (intent_code == NIFTI_INTENT_CORREL || intent_code == NIFTI_INTENT_TTEST || intent_code == NIFTI_INTENT_FTEST ||
      intent_code == NIFTI_INTENT_ZSCORE || intent_code == NIFTI_INTENT_CHISQ || intent_code == NIFTI_INTENT_BETA ||
      intent_code == NIFTI_INTENT_BINOM || intent_code == NIFTI_INTENT_GAMMA || intent_code == NIFTI_INTENT_POISSON ||
      intent_code == NIFTI_INTENT_NORMAL || intent_code == NIFTI_INTENT_FTEST_NONC ||
      intent_code == NIFTI_INTENT_CHISQ_NONC || intent_code == NIFTI_INTENT_LOGISTIC ||
      intent_code == NIFTI_INTENT_LAPLACE || intent_code == NIFTI_INTENT_UNIFORM ||
      intent_code == NIFTI_INTENT_TTEST_NONC || intent_code == NIFTI_INTENT_WEIBULL ||
      intent_code == NIFTI_INTENT_CHI || intent_code == NIFTI_INTENT_INVGAUSS || intent_code == NIFTI_INTENT_EXTVAL ||
      intent_code == NIFTI_INTENT_PVAL || intent_code == NIFTI_INTENT_LOGPVAL ||
      intent_code == NIFTI_INTENT_LOG10PVAL || intent_code == NIFTI_INTENT_ESTIMATE) {
    int error = MRISwriteGIFTIStats(mris, image, intent_code); 
    if (error != NO_ERROR)
      return error;
  }  // end of if NIFTI_INTENT_<stats>

  return NO_ERROR;
} // end of MRISwriteGIFTIIntent(MRIS *mris, ...) 


/*
 * Shape file
 *       intent_code = NIFTI_INTENT_SHAPE
 */
int MRISwriteGIFTIShape(MRIS *mris, gifti_image *image, int intent_code, const char *curv_fname)
{
    // ??? input mris should have curvature information ???
    /// ??? why read again ??? 
    if (MRISreadCurvatureFile(mris, curv_fname)) {
      fprintf(stderr, "MRISwriteGIFTIShape: couldn't read %s\n", curv_fname);
      gifti_free_image(image);
      return ERROR_BADFILE;
    }

    giiDataArray *shape = gifti_alloc_and_add_darray(image);
    if (NULL == shape) {
      fprintf(stderr, "MRISwriteGIFTIShape: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    shape->intent = NIFTI_INTENT_SHAPE;
    shape->datatype = NIFTI_TYPE_FLOAT32;
    shape->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    shape->num_dim = 1;
    shape->dims[0] = mris->nvertices;
    shape->dims[1] = 0;
    shape->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    shape->endian = GIFTI_ENDIAN_LITTLE;
#else
    shape->endian = GIFTI_ENDIAN_BIG;
#endif
    shape->coordsys = NULL;
    shape->nvals = gifti_darray_nvals(shape);
    gifti_datatype_sizes(shape->datatype, &shape->nbyper, NULL);

    /* include some metadata describing this shape */
    gifti_add_to_meta(&shape->meta, "Name", curv_fname, 1);
    const char *meta = NULL;
    if (strstr(curv_fname, ".thickness")) {
      meta = "Thickness";
    }
    if (strstr(curv_fname, ".curv")) {
      meta = "CurvatureRadial";
    }
    if (strstr(curv_fname, ".sulc")) {
      meta = "SulcalDepth";
    }
    if (strstr(curv_fname, ".area")) {
      meta = "Area";
    }
    if (strstr(curv_fname, ".volume")) {
      meta = "Volume";
    }
    if (strstr(curv_fname, ".jacobian")) {
      meta = "Jacobian";
    }
    if (meta) {
      gifti_add_to_meta(&shape->meta, "ShapeDataType", meta, 1);
    }

    /* Allocate the data array. */
    shape->data = NULL;
    shape->data = (void *)calloc(shape->nvals, shape->nbyper);
    if (NULL == shape->data) {
      fprintf(stderr,
              "MRISwriteGIFTIShape: couldn't allocate shape data of "
              "length %d, element size %d\n",
              (int)shape->nvals,
              shape->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our data. */
    int vno;
    for (vno = 0; vno < mris->nvertices; vno++) {
      if (mris->vertices[vno].ripflag) {
        continue;
      }
      gifti_set_DA_value_2D(shape, vno, 0, mris->vertices[vno].curv);
    }

    return NO_ERROR;
} // end of MRISwriteGIFTIShape(MRIS *mris, ...)


/*
 * Statistics file
 *       intent_code = NIFTI_INTENT_<stats>
 */
int MRISwriteGIFTIStats(MRIS *mris, gifti_image *image, int intent_code)
{
    giiDataArray *stats = gifti_alloc_and_add_darray(image);
    if (NULL == stats) {
      fprintf(stderr, "MRISwriteGIFTIStats: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    stats->intent = intent_code;
    stats->datatype = NIFTI_TYPE_FLOAT32;
    stats->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    stats->num_dim = 1;
    stats->dims[0] = mris->nvertices;
    stats->dims[1] = 0;
    stats->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    stats->endian = GIFTI_ENDIAN_LITTLE;
#else
    stats->endian = GIFTI_ENDIAN_BIG;
#endif
    stats->coordsys = NULL;
    stats->nvals = gifti_darray_nvals(stats);
    gifti_datatype_sizes(stats->datatype, &stats->nbyper, NULL);

    /* include some metadata describing this thing */
    gifti_add_to_meta(&stats->meta, "Intent_code", gifti_intent_to_string(intent_code), 1);
    if (intent_code == NIFTI_INTENT_UNIFORM) {
      gifti_add_to_meta(&stats->meta, "Intent_p1", "0", 1);  // lower end
      gifti_add_to_meta(&stats->meta, "Intent_p2", "1", 1);  // upper end
    }

    /* Allocate the data array. */
    stats->data = NULL;
    stats->data = (void *)calloc(stats->nvals, stats->nbyper);
    if (NULL == stats->data) {
      fprintf(stderr,
              "MRISwriteGIFTIStats: couldn't allocate stats data of "
              "length %d, element size %d\n",
              (int)stats->nvals,
              stats->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our data. */
    int vno;
    for (vno = 0; vno < mris->nvertices; vno++) {
      if (mris->vertices[vno].ripflag) {
        continue;
      }
      gifti_set_DA_value_2D(stats, vno, 0, mris->vertices[vno].stat);
    }

    return NO_ERROR;
} // end of MRISwriteGIFTIStats(MRIS *mris, ...)


/*
 * Label file
 *       intent_code = NIFTI_INTENT_LABEL
 */
int MRISwriteGIFTILabel(MRIS *mris, gifti_image *image, int intent_code)
{
    /*
     * Writes .annot data to a label table data gifti file:
     * puts the freesurfer colortable struct into a LabelTable,
     * and puts the .annotation field from each vertex into a
     * DataArray
     */
    /*
     * LabelTable struct, fill it in with our colortable stuff
     */
    giiLabelTable labeltable;
    labeltable.length = mris->ct->nentries;
    if (labeltable.length == 0) {
      fprintf(stderr, "MRISwriteGIFTILabel: colortable is empty!\n");
      return ERROR_BADFILE;
    }
    labeltable.key = (int *)calloc(labeltable.length, sizeof(int *));
    labeltable.label = (char **)calloc(labeltable.length, sizeof(char *));
    labeltable.rgba = (float *)calloc(labeltable.length, 4 * sizeof(float *));
    if ((NULL == labeltable.key) || (NULL == labeltable.label) || (NULL == labeltable.rgba)) {
      fprintf(stderr, "MRISwriteGIFTILabel: couldn't allocate giftiLabelTable\n");
      return ERROR_NOMEMORY;
    }
    float *rgba = labeltable.rgba;
    int idx;
    for (idx = 0; idx < labeltable.length; idx++) {
      // the key could be the freesurfer 'annotation' value, which is
      // supposed to be unique to the FreeSurferColorLUT, but for gifti
      // purposes, it is more intutive and obvious to use the index.
      // also, a display application might choose to interpret the
      // label data at each vertex as indicies rather than keys (which
      // i think ignores the gifti spec, but is reasonable to do so).
      labeltable.key[idx] = idx;
      // labeltable.key[idx] = CTABrgb2Annotation(mris->ct->entries[idx]->ri,
      //                                       mris->ct->entries[idx]->gi,
      //                                       mris->ct->entries[idx]->bi);
      // printf("%8.8X\n",labeltable.key[idx]);

      if (strlen(mris->ct->entries[idx]->name) != 0) {
        // printf("idx=%d, name=%s\n",idx,mris->ct->entries[idx]->name);
        labeltable.label[idx] = strcpyalloc(mris->ct->entries[idx]->name);
      }
      else {
        char tmpname[30];
        sprintf(tmpname, "unknown_%d", idx);
        printf("idx=%d, name=NULL, assigned as %s (is the colortable correct?)\n", idx, tmpname);
        labeltable.label[idx] = strcpyalloc(tmpname);
      }

      if ((strlen(mris->ct->entries[idx]->name) == 0) ||
          (strcmp(labeltable.label[idx], "unknown") == 0) ||
          (strcmp(labeltable.label[idx], "Unknown") == 0)) {
        // make certain unknown region is completely empty, invisible
        rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0.0f;
      }
      else {
        rgba[0] = mris->ct->entries[idx]->rf;
        rgba[1] = mris->ct->entries[idx]->gf;
        rgba[2] = mris->ct->entries[idx]->bf;
        rgba[3] = 1.0f;
      }
      rgba += 4;  // next color
      /*
        printf("RGBA: %d %d %d %d %f %f %f %f\n",
        mris->ct->entries[idx]->ri,
        mris->ct->entries[idx]->gi,
        mris->ct->entries[idx]->bi,
        mris->ct->entries[idx]->ai,
        mris->ct->entries[idx]->rf,
        mris->ct->entries[idx]->gf,
        mris->ct->entries[idx]->bf,
        mris->ct->entries[idx]->af);
      */
    }
    // dont forget to stick us in the image
    image->labeltable = labeltable;
    // gifti_disp_LabelTable(NULL,&image->labeltable);

    /*
     * Labels array
     */
    giiDataArray *labels = gifti_alloc_and_add_darray(image);
    if (NULL == labels) {
      fprintf(stderr, "MRISwriteGIFTILabel: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    labels->intent = NIFTI_INTENT_LABEL;
    labels->datatype = NIFTI_TYPE_INT32;
    labels->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    labels->num_dim = 1;
    labels->dims[0] = mris->nvertices;
    labels->dims[1] = 0;
    labels->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    labels->endian = GIFTI_ENDIAN_LITTLE;
#else
    labels->endian = GIFTI_ENDIAN_BIG;
#endif
    labels->coordsys = NULL;
    labels->nvals = gifti_darray_nvals(labels);
    gifti_datatype_sizes(labels->datatype, &labels->nbyper, NULL);

    /* include some metadata describing this as a label */
    gifti_add_to_meta(&labels->meta, "Name", "node label", 1);
    if (labeltable.length == 2) {
      // in the special case of a label table consisting of one label
      // (assuming the first label is 'unknown') use this one label as name,
      // for instance in the case of the V1 label
      gifti_add_to_meta(&labels->meta, "Name", labeltable.label[1], 1);
    }

    /* Allocate the data array. */
    labels->data = NULL;
    labels->data = (void *)calloc(labels->nvals, labels->nbyper);
    if (NULL == labels->data) {
      fprintf(stderr,
              "MRISwriteGIFTILabel: couldn't allocate labels data of "
              "length %d, element size %d\n",
              (int)labels->nvals,
              labels->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy our 'annotation' data for each vertex (actually an index) */
    unsigned int *label_data = (unsigned int *)labels->data;
    int label_index, theIdx, result;
    for (label_index = 0; label_index < mris->nvertices; label_index++) {
      if (mris->vertices[label_index].ripflag) {
        continue;
      }
      result = CTABfindAnnotation(mris->ct, mris->vertices[label_index].annotation, &theIdx);
      if (result) {
        return ERROR_BADFILE;
      }

      *(label_data + label_index) = theIdx;
      // printf("%8.8X ", *(label_data + label_index));
    }

    return NO_ERROR;
} // end of MRISwriteGIFTILabel()


/*
 * Surface file
 *       output NIFTI_INTENT_POINTSET or NIFTI_INTENT_TRIANGLE
 */
int MRISwriteGIFTISurface(MRIS *mris, gifti_image *image, const char *out_fname)
{
    /*
     * Coordinates
     */
    giiDataArray *coords = gifti_alloc_and_add_darray(image);
    if (NULL == coords) {
      fprintf(stderr, "MRISwriteGIFTISurface: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    coords->intent = NIFTI_INTENT_POINTSET;
    coords->datatype = NIFTI_TYPE_FLOAT32;
    coords->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    coords->num_dim = 2;
    coords->dims[0] = mris->nvertices;        /* In highest first, dim0 = rows */
    coords->dims[1] = 3;                      /* In highest first, dim1 = cols */
    coords->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    coords->endian = GIFTI_ENDIAN_LITTLE;
#else
    coords->endian = GIFTI_ENDIAN_BIG;
#endif

    /* The implementation as of 12/19/2022 set
     * <DataSpace> = NIFTI_XFORM_UNKNOWN
     * <MatrixData> = mris->SRASToTalSRAS_
     * <TransformedSpace> = NIFTI_XFORM_TALAIRACH
     */
    coords->coordsys = NULL;             // empty, unless we find something here...

    if (mris->useRealRAS)
    {
      // surface XYZ coordinates are in scanner space
      if (mris->vg.valid)
      {
        MATRIX *S = vg_i_to_r(&mris->vg);
        MATRIX *T = TkrVox2RASfromVolGeom(&mris->vg);
        MATRIX *Sinv = MatrixInverse(S, NULL);
        MATRIX *xform = MatrixMultiply(T, Sinv, NULL);

        gifti_add_empty_CS(coords);
        int idx = coords->numCS - 1;

        //  <DataSpace> = NIFTI_XFORM_SCANNER_ANAT
        //  <MatrixData> = transform matrix go from scanner space to Freesurfer tkregister space
        //  <TransformedSpace> = NIFTI_XFORM_UNKNOWN (Freesurfer tkregister space)
        coords->coordsys[idx]->dataspace = strcpyalloc("NIFTI_XFORM_SCANNER_ANAT");
        coords->coordsys[idx]->xformspace = strcpyalloc("NIFTI_XFORM_UNKNOWN");

        for (int r = 1; r <= 4; r++)
          for (int c = 1; c <= 4; c++)
            coords->coordsys[idx]->xform[r - 1][c - 1] = xform->rptr[r][c];

        MatrixFree(&S);
        MatrixFree(&T);
        MatrixFree(&Sinv);
        MatrixFree(&xform);
      }
      else
      {
        gifti_add_empty_CS(coords);
        int idx = coords->numCS - 1;

        coords->coordsys[idx]->dataspace = strcpyalloc("NIFTI_XFORM_SCANNER_ANAT");
        coords->coordsys[idx]->xformspace = strcpyalloc("NIFTI_XFORM_SCANNER_ANAT");

        MATRIX *xform = MatrixIdentity(4, NULL);
        for (int r = 1; r <= 4; r++)
          for (int c = 1; c <= 4; c++)
            coords->coordsys[idx]->xform[r - 1][c - 1] = xform->rptr[r][c];

        MatrixFree(&xform);
      }
    }
    else
    {
      // surface XYZ coordinates are in tkregister space
      if (mris->vg.valid)
      {
        MATRIX *S = vg_i_to_r(&mris->vg);
        MATRIX *T = TkrVox2RASfromVolGeom(&mris->vg);
        MATRIX *Tinv = MatrixInverse(T, NULL);
        MATRIX *xform = MatrixMultiply(S, Tinv, NULL);

        gifti_add_empty_CS(coords);
        int idx = coords->numCS - 1;

        //  <DataSpace> = NIFTI_XFORM_UNKNOWN (Freesurfer tkregister space)
        //  <MatrixData> = transform matrix go from Freesurfer tkregister space to scanner space
        //  <TransformedSpace> = NIFTI_XFORM_SCANNER_ANAT
        coords->coordsys[idx]->dataspace = strcpyalloc("NIFTI_XFORM_UNKNOWN");
        coords->coordsys[idx]->xformspace = strcpyalloc("NIFTI_XFORM_SCANNER_ANAT");

        for (int r = 1; r <= 4; r++)
          for (int c = 1; c <= 4; c++)
            coords->coordsys[idx]->xform[r - 1][c - 1] = xform->rptr[r][c];

        MatrixFree(&S);
        MatrixFree(&T);
        MatrixFree(&Tinv);
        MatrixFree(&xform);
      }
      else
      {
        // ??? read into a local MRIS ???
        MRISreadTransform(mris, out_fname);  // tries to get xform from out_fname
        if (mris->SRASToTalSRAS_ && mris->SRASToTalSRAS_->rows == 4 && mris->SRASToTalSRAS_->cols == 4) {
          gifti_add_empty_CS(coords);
          int idx = coords->numCS - 1;
          // found a valid xform, so use it...
          coords->coordsys[idx]->dataspace = strcpyalloc("NIFTI_XFORM_UNKNOWN");
          coords->coordsys[idx]->xformspace = strcpyalloc("NIFTI_XFORM_TALAIRACH");
          MATRIX *xform = mris->SRASToTalSRAS_;
          int r, c;
          for (r = 1; r <= 4; r++)
            for (c = 1; c <= 4; c++) {
              coords->coordsys[idx]->xform[r - 1][c - 1] = xform->rptr[r][c];
            }
          }
      }
    }

    coords->nvals = gifti_darray_nvals(coords);
    gifti_datatype_sizes(coords->datatype, &coords->nbyper, NULL);

    /* Allocate the data array. */
    coords->data = NULL;
    coords->data = (void *)calloc(coords->nvals, coords->nbyper);
    if (NULL == coords->data) {
      fprintf(stderr,
              "MRISwriteGIFTISurface: couldn't allocate coords data of "
              "length %d, element size %d\n",
              (int)coords->nvals,
              coords->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our data. */
    int vertex_index;
    for (vertex_index = 0; vertex_index < mris->nvertices; vertex_index++) {
      if (mris->vertices[vertex_index].ripflag) {
        continue;
      }
      gifti_set_DA_value_2D(coords, vertex_index, 0, mris->vertices[vertex_index].x);
      gifti_set_DA_value_2D(coords, vertex_index, 1, mris->vertices[vertex_index].y);
      gifti_set_DA_value_2D(coords, vertex_index, 2, mris->vertices[vertex_index].z);
    }

    /*
     * Faces
     */
    giiDataArray *faces = gifti_alloc_and_add_darray(image);
    if (NULL == faces) {
      fprintf(stderr, "MRISwriteGIFTISurface: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* count the real number of faces (the ones that dont have a vertex
       with a ripflag set) */
    int numFaces = 0;
    int face_index;
    for (face_index = 0; face_index < mris->nfaces; face_index++) {
      if (mris->vertices[mris->faces[face_index].v[0]].ripflag) {
        continue;
      }
      if (mris->vertices[mris->faces[face_index].v[1]].ripflag) {
        continue;
      }
      if (mris->vertices[mris->faces[face_index].v[2]].ripflag) {
        continue;
      }
      numFaces++;
    }

    /* Set its attributes. */
    faces->intent = NIFTI_INTENT_TRIANGLE;
    faces->datatype = NIFTI_TYPE_INT32;
    faces->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    faces->num_dim = 2;
    faces->dims[0] = numFaces;               /* In highest first, dim0 = rows */
    faces->dims[1] = 3;                      /* In highest first, dim1 = cols */
    faces->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    faces->endian = GIFTI_ENDIAN_LITTLE;
#else
    faces->endian = GIFTI_ENDIAN_BIG;
#endif
    faces->coordsys = NULL;
    faces->nvals = gifti_darray_nvals(faces);
    gifti_datatype_sizes(faces->datatype, &faces->nbyper, NULL);

    /* Allocate the data array. */
    faces->data = NULL;
    faces->data = (void *)calloc(faces->nvals, faces->nbyper);
    if (NULL == faces->data) {
      fprintf(stderr,
              "MRISwriteGIFTISurface: couldn't allocate faces data of "
              "length %d, element size %d\n",
              (int)faces->nvals,
              faces->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our face data (remembering to ignore faces which
       have a vertex with the ripflag set). */
    int faceNum = 0;
    for (face_index = 0; face_index < mris->nfaces; face_index++) {
      if (mris->vertices[mris->faces[face_index].v[0]].ripflag) {
        continue;
      }
      if (mris->vertices[mris->faces[face_index].v[1]].ripflag) {
        continue;
      }
      if (mris->vertices[mris->faces[face_index].v[2]].ripflag) {
        continue;
      }

      gifti_set_DA_value_2D(faces, faceNum, 0, mris->faces[face_index].v[0]);
      gifti_set_DA_value_2D(faces, faceNum, 1, mris->faces[face_index].v[1]);
      gifti_set_DA_value_2D(faces, faceNum, 2, mris->faces[face_index].v[2]);
      faceNum++;
    }

    /* standard meta data for surfaces */
    //printf("[DEBUG] MRISwriteGIFTISurface(): mris->fname=%s\n", (strlen(mris->fname) != 0) ? mris->fname : "null");
    if (strlen(mris->fname) != 0) {
      //printf("[DEBUG] MRISwriteGIFTISurface(): writing standard metadata ...\n");
      const char *primary = NULL, *secondary = NULL, *geotype = NULL;
      char *name = mris->fname;
      if (strstr(name, "lh.")) {
        primary = "CortexLeft";
      }
      if (strstr(name, "rh.")) {
        primary = "CortexRight";
      }
      if (strstr(name, ".orig")) {
        secondary = "GrayWhite";
      }
      if (strstr(name, ".smoothwm")) {
        secondary = "GrayWhite";
      }
      if (strstr(name, ".white")) {
        secondary = "GrayWhite";
      }
      if (strstr(name, ".graymid")) {
        secondary = "MidThickness";
      }
      if (strstr(name, ".gray")) {
        secondary = "Pial";
      }
      if (strstr(name, ".pial")) {
        secondary = "Pial";
      }
      if (strstr(name, ".orig")) {
        geotype = "Reconstruction";
      }
      if (strstr(name, ".smoothwm")) {
        geotype = "Reconstruction";  //"Anatomical";
      }
      if (strstr(name, ".white")) {
        geotype = "Anatomical";
      }
      if (strstr(name, ".gray")) {
        geotype = "Anatomical";
      }
      if (strstr(name, ".graymid")) {
        geotype = "Anatomical";
      }
      if (strstr(name, ".pial")) {
        geotype = "Anatomical";
      }
      if (strstr(name, ".inflated")) {
        geotype = "Inflated";
      }
      if (strstr(name, ".sphere")) {
        geotype = "Sphere";
      }
      if (strstr(name, ".qsphere")) {
        geotype = "Sphere";
      }
      if (strstr(name, "pial-outer")) {
        geotype = "Hull";
      }
      const char *topotype;
      if (mris->patch) {
        //geotype = "Flat";
        topotype = "Cut";
      } else {
        topotype = "Closed";
      }

      if (primary) gifti_add_to_meta(&coords->meta, "AnatomicalStructurePrimary", primary, 1);
      if (secondary) gifti_add_to_meta(&coords->meta, "AnatomicalStructureSecondary", secondary, 1);
      if (geotype) gifti_add_to_meta(&coords->meta, "GeometricType", geotype, 1);
      gifti_add_to_meta(&faces->meta, "TopologicalType", topotype, 1);
      gifti_add_to_meta(&coords->meta, "Name", name, 1);
      gifti_add_to_meta(&faces->meta, "Name", name, 1);
    } // end writing standard gifti metadata

    // add volume geometry info if valid, and surface center-coords
    if (mris->vg.valid) {
      char stmp[100];

      sprintf(stmp, "%d", mris->vg.width);
      gifti_add_to_meta(&coords->meta, "VolGeomWidth", stmp, 1);
      sprintf(stmp, "%d", mris->vg.height);
      gifti_add_to_meta(&coords->meta, "VolGeomHeight", stmp, 1);
      sprintf(stmp, "%d", mris->vg.depth);
      gifti_add_to_meta(&coords->meta, "VolGeomDepth", stmp, 1);

      sprintf(stmp, "%f", mris->vg.xsize);
      gifti_add_to_meta(&coords->meta, "VolGeomXsize", stmp, 1);
      sprintf(stmp, "%f", mris->vg.ysize);
      gifti_add_to_meta(&coords->meta, "VolGeomYsize", stmp, 1);
      sprintf(stmp, "%f", mris->vg.zsize);
      gifti_add_to_meta(&coords->meta, "VolGeomZsize", stmp, 1);

      sprintf(stmp, "%f", mris->vg.x_r);
      gifti_add_to_meta(&coords->meta, "VolGeomX_R", stmp, 1);
      sprintf(stmp, "%f", mris->vg.x_a);
      gifti_add_to_meta(&coords->meta, "VolGeomX_A", stmp, 1);
      sprintf(stmp, "%f", mris->vg.x_s);
      gifti_add_to_meta(&coords->meta, "VolGeomX_S", stmp, 1);

      sprintf(stmp, "%f", mris->vg.y_r);
      gifti_add_to_meta(&coords->meta, "VolGeomY_R", stmp, 1);
      sprintf(stmp, "%f", mris->vg.y_a);
      gifti_add_to_meta(&coords->meta, "VolGeomY_A", stmp, 1);
      sprintf(stmp, "%f", mris->vg.y_s);
      gifti_add_to_meta(&coords->meta, "VolGeomY_S", stmp, 1);

      sprintf(stmp, "%f", mris->vg.z_r);
      gifti_add_to_meta(&coords->meta, "VolGeomZ_R", stmp, 1);
      sprintf(stmp, "%f", mris->vg.z_a);
      gifti_add_to_meta(&coords->meta, "VolGeomZ_A", stmp, 1);
      sprintf(stmp, "%f", mris->vg.z_s);
      gifti_add_to_meta(&coords->meta, "VolGeomZ_S", stmp, 1);

      sprintf(stmp, "%f", mris->vg.c_r);
      gifti_add_to_meta(&coords->meta, "VolGeomC_R", stmp, 1);
      sprintf(stmp, "%f", mris->vg.c_a);
      gifti_add_to_meta(&coords->meta, "VolGeomC_A", stmp, 1);
      sprintf(stmp, "%f", mris->vg.c_s);
      gifti_add_to_meta(&coords->meta, "VolGeomC_S", stmp, 1);

      sprintf(stmp, "%f", mris->xctr);
      gifti_add_to_meta(&coords->meta, "SurfaceCenterX", stmp, 1);
      sprintf(stmp, "%f", mris->yctr);
      gifti_add_to_meta(&coords->meta, "SurfaceCenterY", stmp, 1);
      sprintf(stmp, "%f", mris->zctr);
      gifti_add_to_meta(&coords->meta, "SurfaceCenterZ", stmp, 1);
    }

    // group avg surface area, TAG_GROUP_AVG_SURFACE_AREA
    if (!FZERO(mris->group_avg_surface_area))
    {
      char group_avg_surface_area[100] = {'\0'};

      sprintf(group_avg_surface_area, "%.20f", mris->group_avg_surface_area);
      gifti_add_to_meta(&coords->meta, "TAG_GROUP_AVG_SURFACE_AREA", group_avg_surface_area, 1);
    }

    // TAG_CMDLINE
    if (mris->ncmds > 0)
    {
      char ncmds[20] = {'\0'};
      sprintf(ncmds, "%d", mris->ncmds);
      gifti_add_to_meta(&coords->meta, "NUM_TAG_CMDLINE", ncmds, 1);

      for (int ncmd = 0; ncmd < mris->ncmds; ncmd++)
      {
        char cmdline[TAG_CMDLINE_LEN] = {'\0'};
        snprintf(cmdline, sizeof(cmdline), "%s", mris->cmdlines[ncmd]);

        char tag[20] = {'\0'};
        sprintf(tag, "%s#%d", "TAG_CMDLINE", ncmd);
        gifti_add_to_meta(&coords->meta, tag, cmdline, 1);
      }
    }

    return NO_ERROR;
} // end of MRISwriteGIFTISurface()



int MRISwriteGIFTICombined(MRIS *mris, MRISurfOverlay *poverlays, const char *out_fname)
{
  if (NULL == mris || NULL == out_fname) {
    fprintf(stderr, "MRISwriteGIFTICombined: invalid parameter\n");
    return ERROR_BADPARM;
  }

  gifti_image *image = (gifti_image *)calloc(1, sizeof(gifti_image));
  if (NULL == image) {
    fprintf(stderr, "MRISwriteGIFTICombined: couldn't allocate image\n");
    return ERROR_NOMEMORY;
  }
  image->version = strcpyalloc(GIFTI_XML_VERSION);

  insertCommonMetaData(&image->meta);
  if (strlen(mris->subject_name)) {
    gifti_add_to_meta(&image->meta, "SubjectID", mris->subject_name, 1);
  }

  // write surface
  int error = MRISwriteGIFTISurface(mris, image, out_fname); 
  if (error != NO_ERROR)
    return error;  

  // write overlays
  MRI *overlaymri = poverlays->getOverlayMRI();
  int noverlay = poverlays->getNumOverlay();
  for (int n = 0; n < noverlay; n++)
  {
    int giftiintent = poverlays->getGIFTIIntent(n);
    int stFrame = poverlays->getFirstFrameNo(n);
    int endFrame = poverlays->getNumFrames(n);
    for (int f = stFrame; f < endFrame; f++)
    {
      const char *datatype = poverlays->getDataType(n);
      int error = MRISwriteGIFTIIntent(mris, overlaymri, f, f+1, image, giftiintent, out_fname, poverlays->getOverlayFilename(n), datatype);
      if (error != NO_ERROR)
        return error;
    }
  }

  // make sure version is recoded before validation
  if (!strcmp(image->version, "1")) {
    free(image->version);
    image->version = strcpyalloc(GIFTI_XML_VERSION);
  }

  /* check for compliance */
  int valid = gifti_valid_gifti_image(image, 1);
  if (valid == 0) {
    fprintf(stderr, "MRISwriteGIFTICombined: GIFTI file %s is invalid!\n", out_fname);
    gifti_free_image(image);
    return ERROR_BADFILE;
  }

  /* Write the file. */
  if (gifti_write_image(image, out_fname, 1)) {
    fprintf(stderr, "MRISwriteGIFTICombined: couldn't write image\n");
    gifti_free_image(image);
    return ERROR_BADFILE;
  }

  gifti_free_image(image);

  return ERROR_NONE;
} // end of MRISwriteGIFTICombined()


int MRISwriteGIFTIIntent(MRIS *mris, const MRI *mri, int stframe, int endframe, gifti_image *image, int intent_code, const char *out_fname, const char *curv_fname, const char *datatype)
{
  /* -------------------------------------------------------
   * Surface file
   */
  if (intent_code == NIFTI_INTENT_POINTSET || intent_code == NIFTI_INTENT_TRIANGLE) {
    return MRISwriteGIFTISurface(mris, image, out_fname); 
  }  // end of if NIFTI_INTENT_POINTSET or NIFTI_INTENT_TRIANGLE

  /* -------------------------------------------------------
   * Shape file
   */
  if (intent_code == NIFTI_INTENT_SHAPE) {
    return MRISwriteGIFTIShape(mris, mri, stframe, endframe, image, intent_code, curv_fname, datatype);
  }  // end of if NIFTI_INTENT_SHAPE

  /* -------------------------------------------------------
   * Label file
   */
  if (intent_code == NIFTI_INTENT_LABEL) {
    return MRISwriteGIFTILabel(mris, image, intent_code); 
  }  // end of if NIFTI_INTENT_LABEL

  /* -------------------------------------------------------
   * Statistics file
   */
  if (intent_code == NIFTI_INTENT_CORREL || intent_code == NIFTI_INTENT_TTEST || intent_code == NIFTI_INTENT_FTEST ||
      intent_code == NIFTI_INTENT_ZSCORE || intent_code == NIFTI_INTENT_CHISQ || intent_code == NIFTI_INTENT_BETA ||
      intent_code == NIFTI_INTENT_BINOM || intent_code == NIFTI_INTENT_GAMMA || intent_code == NIFTI_INTENT_POISSON ||
      intent_code == NIFTI_INTENT_NORMAL || intent_code == NIFTI_INTENT_FTEST_NONC ||
      intent_code == NIFTI_INTENT_CHISQ_NONC || intent_code == NIFTI_INTENT_LOGISTIC ||
      intent_code == NIFTI_INTENT_LAPLACE || intent_code == NIFTI_INTENT_UNIFORM ||
      intent_code == NIFTI_INTENT_TTEST_NONC || intent_code == NIFTI_INTENT_WEIBULL ||
      intent_code == NIFTI_INTENT_CHI || intent_code == NIFTI_INTENT_INVGAUSS || intent_code == NIFTI_INTENT_EXTVAL ||
      intent_code == NIFTI_INTENT_PVAL || intent_code == NIFTI_INTENT_LOGPVAL ||
      intent_code == NIFTI_INTENT_LOG10PVAL || intent_code == NIFTI_INTENT_ESTIMATE) {
    return MRISwriteGIFTIStats(mris, mri, stframe, endframe, image, intent_code, curv_fname, datatype); 
  }  // end of if NIFTI_INTENT_<stats>

  return NO_ERROR;
} // end of MRISwriteGIFTIIntent(MRIS *mris, const MRI *mri, ...)


/*
 * Shape file
 *       intent_code = NIFTI_INTENT_SHAPE
 */
int MRISwriteGIFTIShape(MRIS *mris, const MRI *mri, int stframe, int endframe, gifti_image *image, int intent_code, const char *curv_fname, const char *datatype)
{
#if 0
    // data is in mri
    // ??? input mris should have curvature information ???
    /// ??? why read again ??? 
    if (MRISreadCurvatureFile(mris, curv_fname)) {
      fprintf(stderr, "MRISwriteGIFTIShape: couldn't read %s\n", curv_fname);
      gifti_free_image(image);
      return ERROR_BADFILE;
    }
#endif

    if ((endframe - stframe) > 1)
    {
      printf("ERROR MRISwriteGIFTIShape() MRI have more than one frame (%d - %d)\n", endframe, stframe);
      return ERROR_BADFILE;
    }

    giiDataArray *shape = gifti_alloc_and_add_darray(image);
    if (NULL == shape) {
      fprintf(stderr, "MRISwriteGIFTIShape: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    shape->intent = NIFTI_INTENT_SHAPE;
    shape->datatype = NIFTI_TYPE_FLOAT32;
    shape->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    shape->num_dim = 1;
    shape->dims[0] = mris->nvertices;
    shape->dims[1] = 0;
    shape->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    shape->endian = GIFTI_ENDIAN_LITTLE;
#else
    shape->endian = GIFTI_ENDIAN_BIG;
#endif
    shape->coordsys = NULL;
    shape->nvals = gifti_darray_nvals(shape);
    gifti_datatype_sizes(shape->datatype, &shape->nbyper, NULL);

    /* include some metadata describing this shape */
    gifti_add_to_meta(&shape->meta, "Name", curv_fname, 1);
    const char *meta = datatype;
#if 0
    if (strstr(curv_fname, ".thickness")) {
      meta = "Thickness";
    }
    if (strstr(curv_fname, ".curv")) {
      meta = "CurvatureRadial";
    }
    if (strstr(curv_fname, ".sulc")) {
      meta = "SulcalDepth";
    }
    if (strstr(curv_fname, ".area")) {
      meta = "Area";
    }
    if (strstr(curv_fname, ".volume")) {
      meta = "Volume";
    }
    if (strstr(curv_fname, ".jacobian")) {
      meta = "Jacobian";
    }
#endif
    if (meta) {
      gifti_add_to_meta(&shape->meta, "ShapeDataType", meta, 1);
    }

    /* Allocate the data array. */
    shape->data = NULL;
    shape->data = (void *)calloc(shape->nvals, shape->nbyper);
    if (NULL == shape->data) {
      fprintf(stderr,
              "MRISwriteGIFTIShape: couldn't allocate shape data of "
              "length %d, element size %d\n",
              (int)shape->nvals,
              shape->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our data. */
    // loop through MRI crs
    for (int f = stframe; f < endframe; f++)
    {
      for (int s = 0; s < mri->depth; s++)
      {
        for (int r = 0; r < mri->height; r++)
        {
          for (int c = 0; c < mri->width; c++)
	  {
            if (mris->vertices[c].ripflag)
              continue;

            float curv = MRIgetVoxVal(mri, c, r, s, f);
            gifti_set_DA_value_2D(shape, c, 0, curv);
          }
        }
      }
    }
#if 0
    int vno;
    for (vno = 0; vno < mris->nvertices; vno++) {
      if (mris->vertices[vno].ripflag) {
        continue;
      }
      gifti_set_DA_value_2D(shape, vno, 0, mris->vertices[vno].curv);
    }
#endif

    return NO_ERROR;
} // end of MRISwriteGIFTIShape(MRIS *mris, const MRI *mri, ...)


/*
 * Statistics file
 *       intent_code = NIFTI_INTENT_<stats>
 */
int MRISwriteGIFTIStats(MRIS *mris, const MRI *mri, int stframe, int endframe, gifti_image *image, int intent_code, const char *curv_fname, const char *datatype)
{
    if ((endframe - stframe) > 1)
    {
      printf("ERROR MRISwriteGIFTIStats() MRI have more than one frame (%d - %d)\n", endframe, stframe);
      return ERROR_BADFILE;
    }

    giiDataArray *stats = gifti_alloc_and_add_darray(image);
    if (NULL == stats) {
      fprintf(stderr, "MRISwriteGIFTIStats: couldn't allocate giiDataArray\n");
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Set its attributes. */
    stats->intent = intent_code;
    stats->datatype = NIFTI_TYPE_FLOAT32;
    stats->ind_ord = GIFTI_IND_ORD_ROW_MAJOR;
    stats->num_dim = 1;
    stats->dims[0] = mris->nvertices;
    stats->dims[1] = 0;
    stats->encoding = GIFTI_ENCODING_B64GZ;  // data stored in gzip'd base64
#if (BYTE_ORDER == LITTLE_ENDIAN)
    stats->endian = GIFTI_ENDIAN_LITTLE;
#else
    stats->endian = GIFTI_ENDIAN_BIG;
#endif
    stats->coordsys = NULL;
    stats->nvals = gifti_darray_nvals(stats);
    gifti_datatype_sizes(stats->datatype, &stats->nbyper, NULL);

    /* include some metadata describing this thing */
    gifti_add_to_meta(&stats->meta, "Intent_code", gifti_intent_to_string(intent_code), 1);
    if (intent_code == NIFTI_INTENT_UNIFORM) {
      gifti_add_to_meta(&stats->meta, "Intent_p1", "0", 1);  // lower end
      gifti_add_to_meta(&stats->meta, "Intent_p2", "1", 1);  // upper end
    }

    /* Allocate the data array. */
    stats->data = NULL;
    stats->data = (void *)calloc(stats->nvals, stats->nbyper);
    if (NULL == stats->data) {
      fprintf(stderr,
              "MRISwriteGIFTIStats: couldn't allocate stats data of "
              "length %d, element size %d\n",
              (int)stats->nvals,
              stats->nbyper);
      gifti_free_image(image);
      return ERROR_NOMEMORY;
    }

    /* Copy in all our data. */
    // loop through MRI crs
    for (int f = stframe; f < endframe; f++)
    {
      for (int s = 0; s < mri->depth; s++)
      {
        for (int r = 0; r < mri->height; r++)
        {
          for (int c = 0; c < mri->width; c++)
	  {
            if (mris->vertices[c].ripflag)
              continue;

            float stat = MRIgetVoxVal(mri, c, r, s, f);
            gifti_set_DA_value_2D(stats, c, 0, stat);
          }
        }
      }
    }
#if 0
    int vno;
    for (vno = 0; vno < mris->nvertices; vno++) {
      if (mris->vertices[vno].ripflag) {
        continue;
      }
      gifti_set_DA_value_2D(stats, vno, 0, mris->vertices[vno].stat);
    }
#endif

    return NO_ERROR;
} // end of MRISwriteGIFTIStats(MRIS *mris, const MRI *mri, ...)


int getShapeStatIntentCount(const char *fgifti)
{
  /*
   * attempt to read the file
   */
  gifti_image *image = gifti_read_image(fgifti, 1);
  if (NULL == image) {
    fprintf(stderr, "getShapeStatIntentCount(): gifti_read_image() returned NULL\n");
    return 0;
  }

  // make sure version is recoded before validation
  if (!strcmp(image->version, "1")) {
    free(image->version);
    image->version = strcpyalloc(GIFTI_XML_VERSION);
  }

  /*
   * check for compliance
   */
  int valid = gifti_valid_gifti_image(image, 1);
  if (valid == 0) {
    fprintf(stderr, "getShapeStatIntentCount(): GIFTI file %s is invalid!\n", fgifti);
    gifti_free_image(image);
    return 0;
  }

  /*
   * Now parse the DataArrays, count NIFTI_INTENT_SHAPE and NIFTI_INTENT_<stat>
   */
  int count = 0;
  int endDAnum = image->numDA;
  for (int numDA = 0; numDA < endDAnum; numDA++) {
    giiDataArray *darray = image->darray[numDA];

    // skip these intents
    if ((darray->intent == NIFTI_INTENT_POINTSET)   || 
        (darray->intent == NIFTI_INTENT_TRIANGLE)   ||
        (darray->intent == NIFTI_INTENT_LABEL)      || 
        (darray->intent == NIFTI_INTENT_GENMATRIX)  ||
        (darray->intent == NIFTI_INTENT_VECTOR)     || 
        (darray->intent == NIFTI_INTENT_RGB_VECTOR) ||
        (darray->intent == NIFTI_INTENT_RGBA_VECTOR))
      continue;

    count++;
  } 

  /*
   * And we're done.
   */
  gifti_free_image(image);

  return count;
} // end of getShapeStatIntentCount()
