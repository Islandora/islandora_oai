<?php

/**
 * @file
 * islandora_oai.admin.inc
 *
 */

/**
 * Menu callback. Admin setting page for islandora_oai.
 */
function islandora_oai_settings() {
  // Needed to fix a jQuery Update bug where file field breaks forms
  drupal_add_js(drupal_get_path('module', 'islandora_oai') . "/js/fix-jquery-update.js", 'theme');
       
  $base_url = $GLOBALS['base_url'];  
  
  $form['#attributes']['enctype'] = 'multipart/form-data'; 
  
  //------------ Group 1: Global Configuration
  $form['oai2_configuration'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configuration'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  $form['oai2_configuration']['oai2_repository_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Repository Name'),
    '#required' => TRUE,
    '#size' => '50',
    '#default_value' =>  variable_get('oai2_repository_name', 'Repository')
  );
  $form['oai2_configuration']['oai2_path'] = array(
    '#type' => 'textfield',
    '#title' => t('The path of the Repository'),
    '#field_prefix' => $base_url . '/',
    '#required' => TRUE,
    '#size' => '50',
    '#default_value' => variable_get('oai2_path', 'oai2'),
    '#description' => 'The path where the OAI-PMH service respond. Es. '. $base_url .'/oai2'
  );
  $form['oai2_configuration']['oai2_repository_identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Repository unique identifier'),
    '#required' => TRUE,
    '#size' => '50',
    '#default_value' => variable_get('oai2_repository_identifier', 'drupal-site.org'),
    '#description' => 'The identifier for this repository. Es. oai:<strong>drupal-site.org</strong>:123.',
  );
  $form['oai2_configuration']['oai2_admin_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Admin Email'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_admin_email', 'admin@example.com')
  );
  $form['oai2_configuration']['oai2_max_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum Response Size'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_max_size', '20'),
    '#description' => 'The maximum number of records to issue per response. If result set is larger than this number,
                      a resumption token will be issued',
  );
  $form['oai2_configuration']['oai2_expire_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Expiration Time'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_expire_time', '86400'),
    '#description' => 'The amount of time a resumption token will remain valid in seconds. Defaults to one day (86400s).',
  );
  $form['oai2_configuration']['oai2_xslt'] = array(
    '#type' => 'textfield',
    '#title' => t('Repository stylesheet XSLT'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_xslt', ''),
    '#description' => 'The XSLT stylesheet for easy browsing the repository without harvester.',
  );
  $form['oai2_configuration']['oai2_date_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr date field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_date_field', 'fgs.lastModifiedDate'),
    '#description' => 'The date field to use.',
  );
  $form['oai2_configuration']['oai2_collection_field'] = array(
    '#type' => 'textarea',
    '#title' => t('Solr RELS-EXT collection field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_collection_field', "rels.isMemberOfCollection\nrels.isMemberOf"),
    '#description' => t('Solr fields used to determine if a doc is a member of a collection.  Can also specify a prefix, in the from "field_name ~ prefix".  Ends up with ORing together the list of predicates above into a list like: $field_name:"$prefix$collection_pid"'),
  );
  $form['oai2_configuration']['oai2_xacml_role_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr XACML role field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_xacml_role_field', 'rels.isViewableByRole'),
    '#description' => 'Field from the RI which is indexed into Solr',
  );
  $form['oai2_configuration']['oai2_xacml_user_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr XACML role field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_xacml_user_field', 'rels.isViewableByUser'),
    '#description' => 'Field from the RI which is indexed into Solr',
  );
  $form['oai2_configuration']['oai2_content_model_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr hasModel field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_content_model_field', 'rels.hasModel'),
    '#description' => 'Field which indexes the RELS-EXT hasModel relationship',
  );
  $form['oai2_configuration']['oai2_exclude_content_models'] = array(
    '#type' => 'textarea',
    '#title' => t('Exclude Content Models'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_exclude_content_models', ''),
    '#description' => 'By default all objects are included in the harvest. This field will let you exclude all objects with a certain content model. For example to exclude all objects with the islandora collection content model you would use: "islandora:collectionCModel"',
  );

  //---------- Group: Metadata Format Configuration
  $form['oai2_metadata'] = array(
    '#type' => 'fieldset',
    '#title' => t('Metadata Format'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#prefix' => '<div id="islandora_oai">',
    '#suffix' => '</div>',
  );
  
  $metadata_format_options = array();
  $result = db_query('SELECT * FROM {oai2_metadata_formats} ORDER BY name');
  while ($row = db_fetch_object($result)) {
    $metadata_format_options[$row->name] = strtoupper($row->name);
  }
  $metadata_format = variable_get('oai2_metadata_format', 'oai_dc');
  $form['oai2_metadata']['oai2_metadata_format'] = array(
    '#type' => 'select',
    '#title' => t('Metadata Format'),
    '#options' => $metadata_format_options,
    '#default_value' =>  $metadata_format,
    '#ahah' => array(
              'path' => "admin/settings/islandora_oai/select",
              'wrapper' => 'islandora_oai',
               ),
    );
  
  $result = db_query("SELECT * FROM {oai2_metadata_formats} where name='%s' ", $metadata_format);
  while ($row = db_fetch_array($result)) {
    $metadata_prefix = $row['metadata_prefix'];
    $metadata_namespace = $row['metadata_namespace'];
    $schema_location = $row['schema'];
  }  
  $form['oai2_metadata']['oai2_metadata_prefix'] = array(
    '#type' => 'item',
    '#title' => t('Metadata Prefix'),
    '#value' =>  $metadata_prefix,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_metadata_namespace'] = array(
    '#type' => 'item',
    '#title' => t('Metadata Namespace'),
    '#value' => $metadata_namespace,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_schema_location'] = array(
    '#type' => 'item',
    '#title' => t('Schema Location'),
    '#value' => $schema_location,
    '#description' => t('Default for selected metadata format.')
  );
  
  $form['oai2_metadata']['oai2_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Transformations'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#prefix' => '<div id="islandora_oai_options">',
    '#suffix' => '</div>',
  );
  
  $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_dc'] = array(
    '#type' => 'textfield',
    '#size' => 30,
    '#title' => 'Metadata Datastream ID',
    '#default_value' => variable_get('oai2_metadata_datastream_id_oai_dc', 'MODS'),
    '#description' => t('Note that the datastream ID is case sensitive'),
  );
    
   $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_etdms'] = array(
    '#type' => 'textfield',
    '#size' => 30,
    '#title' => 'Metadata Datastream ID',
    '#default_value' => variable_get('oai2_metadata_datastream_id_oai_etdms', 'MODS'),
    '#description' => t('Note that the datastream ID is case sensitive'), 
  );
    
  $oai_invoke_files = module_invoke_all('islandora_oai_get_xsl_files');
      
  $transform_options = array('default' => t('No transformation selected'));
  $transform_options = array_merge($transform_options, $oai_invoke_files);
  
  $default_transform_dc = variable_get('oai2_transform_file_oai_dc', drupal_get_path('module', 'islandora_oai') . '/xsl/MODS3-22simpleDC.xsl');
  $default_transform_etdms = variable_get('oai2_transform_file_oai_etdms', drupal_get_path('module', 'islandora_oai') . '/xsl/mods_to_etdms.xsl');
  
  $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_dc'] = array(
    '#type' => 'select',
    '#title' => t('File to use for transforming oai_dc'),
    '#options' => $transform_options,
    '#default_value' => $default_transform_dc,
  );
  $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_etdms'] = array(
    '#type' => 'select',
    '#title' => t('File to use for transforming oai_etdms'),
    '#options' => $transform_options,
    '#default_value' => $default_transform_etdms,
  );
  
  $form['oai2_metadata']['oai2_options']['upload'] = array(
    '#type' => 'fieldset',
    '#title' => t('Upload a file'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,  
  );
  
  $form['oai2_metadata']['oai2_options']['upload']['oai2_upload_xsl'] = array(
    '#type' => 'file',
    '#title_display' => 'invisible',
    '#title' => t('Upload a XSL or XSLT to be used for transformations'),
    '#size' => 22,
  );
  
  $form['oai2_metadata']['oai2_options']['upload']['oai2_upload_xsl_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Upload file'),
    '#ahah' => array(
              'path' => "admin/settings/islandora_oai/upload",
              'wrapper' => 'islandora_oai_options',
              'event' => 'click',
               ),
  );
    
  if ($metadata_prefix == 'oai_dc') {
    $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_etdms']['#type'] = 'hidden';
    $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_etdms']['#type'] = 'hidden';
  }
  else {
    $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_dc']['#type'] = 'hidden';
    $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_dc']['#type'] = 'hidden';
  }
  /** for each content type selected
   *   - list all fields with a form field for DCMI Metadata Term
   */
  
  //---------- Group: Field Mappings
  $form['oai2_field_mapping'] = array(
    '#type' => 'fieldset',
    '#title' => t('Mappings: Field -> DCMI Metadata Term'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t("This is where content type fields are mapped to DCMI Metadata terms. Blank fields will not appear
                         in the OAI-PMH response. If you see no fields here, you must expose some content types above.
                         Field values should be in the form of 'dc:title'<br/><br/>
                         
                        Notes:&nbsp;&nbsp;List of properties in the legacy /elements/1.1/ namespace of Dublin Core:contributor, coverage, creator,
                        date, description, format, identifiers, language, publisher, relation, rights, source, subject, title.<br/><br/>
                        For more information please visit:http://www.dublincore.org/documents/dcmi-terms/<br/><br/>
    "), 
  );
  
  //here we are going to output form fields for every field of each content type we are exposing
  //first get all available content types
  //don't need this as we don't rely on content types anymore we query fedora
 /* $content_types = variable_get('oai2_content_types', array());
  foreach ($content_types as $type => $selected) {
    
    //if we have exposed a content type
    if ($selected) {
      
      //get details about that content type and it's fields
      $content_type = content_types($type);
      $fields = $content_type['fields'];
          
      //put a bit of markup header to separate the content types visually - should maybe be themeable
      $form['oai2_field_mapping']['oai2_type_'. $type]['oai2_markup_'. $type] = array(
        '#value' => "<br/><br/><h3>$type</h3><hr/>",
      );

      //add the body field to the fields array because fields array only contains cck fields
      if (!empty($content_type['extra']['body_field']['label'])) {
        $body_field = array();
        $body_field['field_name'] = 'body';
        $body_field['widget']['label'] = $content_type['extra']['body_field']['label'];
        array_unshift($fields, $body_field);          
      }
      
      //add the title field to the fields array because fields array only contains cck fields      
      if (!empty($content_type['extra']['title']['label'])) {
        $title_field['field_name'] = 'title';
        $title_field['widget']['label'] = $content_type['extra']['title']['label'];
        array_unshift($fields, $title_field);
      }  
      
      //for each field, create a form field with a key like oai2_map:content_type:field_name
      foreach ($fields as $field) {

        $field_name = $field['field_name'];
               
        $form['oai2_field_mapping']['oai2_map:'. $type .':'. $field_name] = array(
          '#type' => 'textfield',
          '#title' => $field['widget']['label'],
          '#size' => '20',
          '#default_value' => variable_get('oai2_map:'. $type .':'. $field_name, ''),          
        );        
      }
    }
    
  }*/
  
  return system_settings_form($form);
}

function islandora_oai_select_metadata_prefix_js() {
  // Retrieve the cached form
  $metadata_format = $_POST['oai2_metadata_format'];
  $result = db_query("SELECT * FROM {oai2_metadata_formats} where name='%s' ", $metadata_format);
  
  $form_state = array('submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  $form = form_get_cache($form_build_id, $form_state);
  
  while ($row = db_fetch_array($result)) {
    $metadata_prefix = $row['metadata_prefix'];
    $metadata_namespace = $row['metadata_namespace'];
    $schema_location = $row['schema'];
  }  
  $form['oai2_metadata']['oai2_metadata_prefix'] = array(
    '#type' => 'item',
    '#title' => t('Metadata Prefix'),
    '#value' =>  $metadata_prefix,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_metadata_namespace'] = array(
    '#type' => 'item',
    '#title' => t('Metadata Namespace'),
    '#value' => $metadata_namespace,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_schema_location'] = array(
    '#type' => 'item',
    '#title' => t('Schema Location'),
    '#value' => $schema_location,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_metadatastream_id']['#value'] = '';
  if ($metadata_prefix == 'oai_dc') {
    $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_etdms']['#type'] = 'hidden';
    $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_dc']['#type'] = 'select';
    $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_etdms']['#type'] = 'hidden';
    $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_dc']['#type'] = 'textfield';
  }
  else {
   $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_dc']['#type'] = 'hidden';
   $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_etdms']['#type'] = 'select';
   $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_etdms']['#type'] = 'textfield';
   $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_oai_dc']['#type'] = 'hidden';
  }
   
  form_set_cache($form_build_id, $form, $form_state);
  $form += array('#post' => $_POST, '#programmed' => FALSE);
  $form = form_builder($_POST['form_id'], $form, $form_state);
  
  $form['oai2_metadata']['#collapsed'] = FALSE;
  
  drupal_json(array(
      'status'   => TRUE,
      'data'     => theme('status_messages') . drupal_render($form['oai2_metadata']),
    ));
  
}

function islandora_oai_upload_js() {
  module_load_include('inc', 'fedora_repository', 'MimeClass');
  
  $form_build_id = $_POST['form_build_id'];
  $form = form_get_cache($form_build_id, $form_state);
  
  $validators = array( 
    'file_validate_extensions' => array('xslt xsl'),
  );

  $upload_path = file_create_path('islandora_oai_xsls');
  $file = file_save_upload('oai2_upload_xsl', $validators, $upload_path, FILE_EXISTS_ERROR);
  
  $mimetype = new MimeClass();
  $mimetype = $mimetype->get_mimetype($file->filename);
  $extension = substr($file->filename, strpos($file->filename, '.'), strlen($file->filename));
 
  if (($extension != '.xsl' && $extension != '.xslt') || $mimetype!= 'text/xml') {
    file_delete($upload_path . '/' . $file->filename);
    form_set_error('oai2_upload_xsl_submit][oai2_options][upload]',t("Upload failed! The file %filename is not a .xsl or .xslt",
      array(
        '%filename' => $file->filename,
      )
    ));
  }
  else {
    if ($file != 0) {
      $options = $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_dc']['#options'];

      file_set_status($file, FILE_STATUS_PERMANENT);

      if (!in_array($file->filename, $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_dc']['#options'])) {
        $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_dc']['#options'][$file->filepath] = $file->filename;
        $form['oai2_metadata']['oai2_options']['oai2_transform_file_oai_etdms']['#options'][$file->filepath] = $file->filename;
        drupal_set_message(t("The file %filename was uploaded successfully", 
                array(
                  '%filename' => $file->filename, 
                )
            ));
      }
    }
    else {
      form_set_error('oai2_upload_xsl_submit][oai2_options][upload][oai2_upload_xsl_submit',t('Please select a file to upload'));
    } 
  }
  form_set_cache($form_build_id, $form, $form_state);
  $form += array('#post' => $_POST, '#programmed' => FALSE);
  $form = form_builder($_POST['form_id'], $form, $form_state);
  
  $form['oai2_metadata']['#collapsed'] = FALSE;
  $form['oai2_metadata']['oai2_options']['#collapsed'] = FALSE;
  drupal_json(array(
      'status'   => TRUE,
      'data'     => theme('status_messages') . drupal_render($form['oai2_metadata']['oai2_options']),
    ));
  exit();
}
