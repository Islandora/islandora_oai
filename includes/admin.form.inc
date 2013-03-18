<?php

/**
 * @file
 * islandora_oai.admin.inc
 *
 */

/**
 * Menu callback. Admin setting page for islandora_oai.
 */
function islandora_oai_settings($form, $form_state) {
  global $base_url;

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
    '#description' => t('The path where the OAI-PMH service respond. Ex. @base_url/oai2', array('@base_url' => $base_url))
  );
  $form['oai2_configuration']['oai2_repository_identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Repository unique identifier'),
    '#required' => TRUE,
    '#size' => '50',
    '#default_value' => variable_get('oai2_repository_identifier', 'drupal-site.org'),
    '#description' => t('The identifier for this repository. Ex. oai:<strong>drupal-site.org</strong>:123.'),
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
    '#description' => t('The maximum number of records to issue per response. If result set is larger than this number,
                      a resumption token will be issued'),
  );
  $form['oai2_configuration']['oai2_expire_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Expiration Time'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_expire_time', '86400'),
    '#description' => t('The amount of time a resumption token will remain valid in seconds. Defaults to one day (86400s).'),
  );
  $form['oai2_configuration']['oai2_date_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr date field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_date_field', 'fgs.lastModifiedDate'),
    '#description' => t('The date field to use.'),
  );
  $form['oai2_configuration']['oai2_collection_field'] = array(
    '#type' => 'textarea',
    '#title' => t('Solr RELS-EXT collection field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_collection_field', "rels.isMemberOfCollection\nrels.isMemberOf"),
    '#description' => t('Solr fields used to determine if a doc is a member of a collection.  To specify a prefix, use this format "field_name ~ prefix".'),
  );
  $form['oai2_configuration']['oai2_xacml_role_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr XACML role field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_xacml_role_field', 'rels.isViewableByRole'),
    '#description' => t('Field from the RI which is indexed into Solr.'),
  );
  $form['oai2_configuration']['oai2_xacml_user_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr XACML user field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_xacml_user_field', 'rels.isViewableByUser'),
    '#description' => t('Field from the RI which is indexed into Solr.'),
  );
  $form['oai2_configuration']['oai2_content_model_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Solr hasModel field'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_content_model_field', 'rels.hasModel'),
    '#description' => t('Field which indexes the RELS-EXT hasModel relationship.'),
  );
  $form['oai2_configuration']['oai2_exclude_content_models'] = array(
    '#type' => 'textarea',
    '#title' => t('Exclude Content Models'),
    '#size' => '50',
    '#default_value' => variable_get('oai2_exclude_content_models', ''),
    '#description' => t('By default all objects are included in the harvest. This field will let you exclude all objects with a certain content model. For example to exclude all objects with the islandora collection content model you would use: "islandora:collectionCModel"'),
  );

  $metadata_format_options = array();
  $result = db_query('SELECT * FROM {oai2_metadata_formats} ORDER BY name');
  foreach ($result as $row) {
    $metadata_format_options[$row->name] = drupal_strtoupper($row->name);
  }

  $metadata_format = variable_get('oai2_metadata_format', 'oai_dc');

  $oai2_options_collapsed = TRUE;
  $oai2_metadata_format_collapsed = TRUE;
  if (isset($form_state['triggering_element'])) {
    if ($form_state['triggering_element']['#name'] == 'oai2_metadata_format') {
      $metadata_format = $form_state['values']['oai2_metadata_format'];
      $oai2_metadata_format_collapsed = FALSE;
    }
    // Upload pressed
    else {
      $oai2_options_collapsed = FALSE;

      $validators = array(
        'file_validate_extensions' => array('xslt', 'xsl'),
      );

      $file = file_save_upload('oai2_upload_xsl', array('file_validate_extensions' => array('xsl xslt')), 'public://islandora_oai_xsls');

      if ($file) {
        $file->status = FILE_STATUS_PERMANENT;
        file_save($file);
        drupal_set_message(t("The file @filename was uploaded successfully.",
          array(
            '@filename' => $file->filename,
          )
        ));
      }
      else {
        drupal_set_message(t('Failed to upload the specified file. Valid extensions are .xsl and .xslt.'), 'error');
      }
    }
  }
  $form['oai2_metadata'] = array(
    '#type' => 'fieldset',
    '#title' => t('Metadata Format'),
    '#collapsible' => TRUE,
    '#collapsed' => $oai2_metadata_format_collapsed,
    '#prefix' => '<div id="islandora_oai">',
    '#suffix' => '</div>',
  );
  $form['oai2_metadata']['oai2_metadata_format'] = array(
    '#type' => 'select',
    '#name' => 'oai2_metadata_format',
    '#title' => t('Metadata Format'),
    '#options' => $metadata_format_options,
    '#default_value' => $metadata_format,
    '#ajax' => array(
      'callback' => 'islandora_oai_select_metadata_prefix_js',
      'wrapper' => 'islandora_oai',
      ),
  );

  $result = db_query("SELECT * FROM {oai2_metadata_formats} WHERE name = :metadata_format",
      array(':metadata_format' => $metadata_format)
  );
  foreach ($result as $row) {
    $metadata_prefix = $row->metadata_prefix;
    $metadata_namespace = $row->metadata_namespace;
    $schema_location = $row->oai2_schema;
  }
  $form['oai2_metadata']['oai2_metadata_prefix'] = array(
    '#type' => 'item',
    '#title' => t('Metadata Prefix'),
    '#markup' =>  $metadata_prefix,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_metadata_namespace'] = array(
    '#type' => 'item',
    '#title' => t('Metadata Namespace'),
    '#markup' => $metadata_namespace,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_schema_location'] = array(
    '#type' => 'item',
    '#title' => t('Schema Location'),
    '#markup' => $schema_location,
    '#description' => t('Default for selected metadata format.')
  );
  $form['oai2_metadata']['oai2_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Transformations'),
    '#collapsible' => TRUE,
    '#collapsed' => $oai2_options_collapsed,
    '#prefix' => '<div id="islandora_oai_options">',
    '#suffix' => '</div>',
  );
  $form['oai2_metadata']['oai2_options']['oai2_metadata_datastream_id_' . $metadata_prefix] = array(
    '#type' => 'textfield',
    '#size' => 30,
    '#title' => 'Metadata Datastream ID',
    '#default_value' => variable_get('oai2_metadata_datastream_id_' . $metadata_prefix, 'MODS'),
    '#description' => t('Note that the datastream ID is case sensitive'),
  );

  $oai_invoke_files = module_invoke_all('islandora_oai_get_xsl_files');
  $transform_options = array('default' => t('No transformation selected'));
  $transform_options = array_merge($transform_options, $oai_invoke_files);

  if ($metadata_prefix == 'oai_dc') {
    $default_transform = variable_get('oai2_transform_file_oai_dc', drupal_get_path('module', 'islandora_oai') . '/xsl/MODS3-22simpleDC.xsl');
  }
  else {
    $default_transform = variable_get('oai2_transform_file_oai_etdms', drupal_get_path('module', 'islandora_oai') . '/xsl/mods_to_etdms.xsl');
  }

  $form['oai2_metadata']['oai2_options']['oai2_transform_file_' . $metadata_prefix] = array(
    '#type' => 'select',
    '#title' => t('File to use for transforming @metadata_prefix', array('@metadata_prefix' => $metadata_prefix)),
    '#options' => $transform_options,
    '#default_value' => $default_transform,
  );

  $form['oai2_metadata']['oai2_options']['upload'] = array(
    '#type' => 'fieldset',
    '#title' => t('Upload a file'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#prefix' => '<div id="islandora_oai_options">',
    '#suffix' => '</div>',
  );

  $form['oai2_metadata']['oai2_options']['upload']['oai2_upload_xsl'] = array(
    '#type' => 'file',
    '#title' => t('Upload a XSL or XSLT to be used for transformations'),
  );

  $form['oai2_metadata']['oai2_options']['upload']['oai2_upload_xsl_button'] = array(
    '#type' => 'button',
    '#name' => 'oai2_upload_button',
    '#value' => t('Upload file'),
    '#ajax' => array(
      'callback' => 'islandora_oai_upload_file_js',
      'wrapper' => 'islandora_oai_options',
     ),
    '#title_display' => 'invisible',
    '#size' => 22,
  );

  return system_settings_form($form);
}

function islandora_oai_select_metadata_prefix_js($form, &$form_state) {
  return $form['oai2_metadata'];
}

function islandora_oai_upload_file_js($form, &$form_state) {
  return $form['oai2_metadata']['oai2_options'];
}

