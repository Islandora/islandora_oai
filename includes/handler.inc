<?php

/**
 * @file
 * Stores all the default handler functionality for OAI responses.
 */

/**
 * Responds to the ListSets request.
 *
 * @param array $params
 *   An array containing:
 *   -token (object): A resumption token object specifying information about
 *   a previous request, or a new empty one.
 *   -max_records (int): The maximum number of records to return per request.
 *
 * @return array
 *   An array containing:
 *   -total_records (int): The total amount of records for the query.
 *   -records (array): An array containing:
 *     -pid (string): The pid of the result
 *     -label (string): The label of the result.
 *
 * @see hook_islandora_oai_identify_request_handler()
 */
function islandora_oai_retrieve_sets($params) {
  $sparql_query = <<<EOQ
  SELECT DISTINCT ?object ?title ?created
  FROM <#ri>
  WHERE {
    {
      ?object <fedora-model:hasModel> ?model
    }
    UNION
    {
      ?object <fedora-model:hasModel> [<fedora-model:hasModel> ?model]
    }
    ?object <fedora-model:label> ?title ;
            <fedora-model:createdDate> ?created .
    !optionals
    !filters
  }
  ORDER BY ?title
EOQ;

  $connection = islandora_get_tuque_connection();

  $optionals = (array) module_invoke('islandora_xacml_api', 'islandora_basic_collection_get_query_optionals', 'view');
  $filter_modules = array(
    'islandora_xacml_api',
    'islandora',
  );
  $filters = array('sameTerm(?model, <info:fedora/islandora:collectionCModel>)');
  $excluded_models = islandora_oai_get_restricted_models();
  if (!empty($excluded_models)) {
    $map_excludes = function ($exclude) {
      return "sameTerm(?model, <info:fedora/$exclude>)";
    };
    $filters[] = format_string('!(!exclude_conjunction)', array(
      '!exclude_conjunction' => implode(' || ', array_map($map_excludes, $excluded_models)),
    ));
  }
  foreach ($filter_modules as $module) {
    $filters = array_merge_recursive($filters, (array) module_invoke($module, 'islandora_basic_collection_get_filters', 'view'));
  }

  $filter_map = function ($filter) {
    return "FILTER($filter)";
  };

  $sparql_query = format_string($sparql_query, array(
    '!optionals' => !empty($optionals) ? ('OPTIONAL {{' . implode('} UNION {', $optionals) . '}}') : '',
    '!filters' => !empty($filters) ? implode(' ', array_map($filter_map, $filters)) : '',
  ));

  $total_records = $connection->repository->ri->countQuery($sparql_query, 'sparql');

  $sparql_query .= <<<EOQ

  LIMIT {$params['max_records']}
  OFFSET {$params['token']->deliveredrecords}
EOQ;
  $records = array();
  $query_results = $connection->repository->ri->sparqlQuery($sparql_query);
  module_load_include('inc', 'islandora', 'includes/dublin_core');
  foreach ($query_results as $result) {
    $object = islandora_object_load($result['object']['value']);
    $description = FALSE;
    if (isset($object['DC'])) {
      $description = $object['DC']->content;
    }
    $pid = $result['object']['value'];
    $records[$pid] = array(
      'pid' => $pid,
      'label' => $result['title']['value'],
    );
    if ($description) {
      $records[$pid]['description'] = $description;
    }
  }
  return array(
    'total_records' => $total_records,
    'records' => $records,
  );
}

/**
 * Constructs the base object response XML depending on configuration options.
 *
 * @param array $params
 *   An array containing:
 *   -metadata_prefix (string): The metadata prefix of the request being
 *   executed.
 *   -pid (string): The pid of the record that we are building for.
 *
 * @return null|string
 *   XML string output of the record depending on configuration, NULL if an
 *   error was encountered.
 *
 * @see hook_islandora_oai_identify_request_handler()
 */
function islandora_oai_object_response_xml($params) {
  $object = islandora_object_load($params['pid']);
  $param = variable_get("islandora_oai_metadata_datastream_id_{$params['metadata_prefix']}", 'DC');
  if (isset($object[$param])) {
    $content = $object[$param]->content;
  }
  else {
    $content = NULL;
  }

  $oai_output = NULL;

  if (!empty($content)) {
    $xslt = new XSLTProcessor();
    $xsl = new DOMDocument();
    $file = variable_get("islandora_oai_transform_file_{$params['metadata_prefix']}", 'default');
    if ($file != 'default') {
      $xsl->load($file);
      $input = new DomDocument();

      $did_load_ok = $input->loadXML($content);

      if ($did_load_ok) {
        $xslt->importStylesheet($xsl);
        $oai_output = $xslt->transformToXml($input);
      }
    }
    else {
      $oai_output = $content;
    }
  }

  // Transformation fails.
  if (empty($oai_output) && isset($object[$param])) {
    watchdog('islandora_oai', 'The pid @pidname failed to transform on the @datastream datastream with the file @file.',
      array(
        '@pidname' => $params['record'],
        '@file' => $file,
        '@datastream' => $param,
      ), WATCHDOG_ERROR);
  }
  // Datastream doesn't exist.
  elseif (!isset($object[$param])) {
    watchdog('islandora_oai', "The pid @pidname failed to transform on the @datastream datastream as the datastream doesn't exist.",
      array(
        '@pidname' => $params['record'],
        '@datastream' => $param,
      ), WATCHDOG_ERROR);
  }
  return $oai_output;
}

/**
 * Handles the GetRecord request.
 *
 * @param array $params
 *   An array containing:
 *   -pid (string): The pid of the record being requested.
 *
 * @return array|bool
 *   FALSE if the record does not exist, otherwise an array containing:
 *   -pid (string): The pid of the record.
 *   -date (string): The date timestamp of the record.
 *   -solr_doc (array): The Solr doc for the record.
 *
 * @see hook_islandora_oai_identify_request_handler()
 */
function islandora_oai_retrieve_record($params) {
  $pid = $params['pid'];
  $record = FALSE;
  $results = islandora_oai_query_solr(0, 'PID', $pid);
  if ($results['numFound'] > 0) {
    $solr_object = $results['objects'][0];
    $record = islandora_oai_construct_record_from_solr_response($solr_object);
  }
  return $record;
}

/**
 * Gets the setSpec membership information for a record.
 *
 * @param array $params
 *   An array containing:
 *   -record: An array containing:
 *     -pid: A string representation of the PID of the record.
 *     -date: A string representation of the date timestamp of the record.
 *     -solr_doc: An array containing the Solr doc for the record.
 *
 * @return array
 *   An array containing all set pids that this record belongs to.
 *
 * @see hook_islandora_oai_identify_request_handler()
 */
function islandora_oai_get_membership($params) {
  $sets = array();
  foreach (islandora_oai_get_membership_array() as $collection_field) {
    if (isset($params['record']['solr_doc'][$collection_field])) {
      // Need to cast to array such that we can check for multiple collections.
      foreach ((array) $params['record']['solr_doc'][$collection_field] as $set) {
        $sets[] = $set;
      }
    }
  }
  return $sets;
}

/**
 * Responds to the ListRecords or ListIdentifier request.
 *
 * @param array $params
 *   An array containing:
 *   -set: String of the PID of the set being searched for, FALSE if no set
 *   argument.
 *   -from: String of the from time starting point, FALSE if no from argument.
 *   -until: String of the until ending point, FALSE if no until argument.
 *   -token: Object representing a token.
 *
 * @return array
 *   An array describing the record retrieved from Solr.
 */
function islandora_oai_retrieve_records_or_identifiers($params) {
  $records = array();
  $set = $params['set'];
  $from = $params['from'];
  $until = $params['until'];
  $token = $params['token'];
  $field = 'PID';
  $query = NULL;
  $date_query = NULL;

  if ($from|| $until) {
    $field = variable_get('islandora_oai_date_field', 'fgs_lastModifiedDate_dt');

    if ($from) {
      $solr_from = islandora_oai_datetime_to_solr_time($from);
    }
    else {
      $solr_from = '*';
    }

    if ($until) {
      $solr_until = islandora_oai_datetime_to_solr_time($until, TRUE);
    }
    else {
      $solr_until = '*';
    }
    $date_query = "$field:[$solr_from TO $solr_until]";
  }

  if (!isset($set)) {
    $set = NULL;
  }
  $results = islandora_oai_query_solr($token->deliveredrecords, $field, $query, $set, $date_query);
  foreach ($results['objects'] as $solr_object) {
    $records[] = islandora_oai_construct_record_from_solr_response($solr_object);
  }
  return array(
    'total_records' => $results['numFound'],
    'records' => $records,
  );
}

/**
 * Helper function to mold the output from the queryProcessor to form.
 *
 * @param array $solr_object
 *   An array describing a result obtained from a Solr query.
 *
 * @return array
 *   An array describing the record retrieved from Solr.
 */
function islandora_oai_construct_record_from_solr_response($solr_object) {
  return array(
    'pid' => $solr_object['PID'],
    'date' => $solr_object['solr_doc'][variable_get('islandora_oai_date_field', 'fgs_lastModifiedDate_dt')],
    'solr_doc' => $solr_object['solr_doc'],
  );
}

/**
 * Converts a time string into a form Solr can parse.
 *
 * @param string $datetime
 *   Time string to be parsed.
 * @param bool $until
 *   Whether an until time is specified.
 *
 * @return string
 *   A string representing the time in the given granularity.
 */
function islandora_oai_datetime_to_solr_time($datetime, $until = FALSE) {
  $granularity = islandora_oai_get_date_granularity($datetime);
  $to_return = $datetime;

  switch ($granularity) {
    case 'YYYY-MM-DD':
      $to_return .= 'T00:00:00Z';
      if ($until) {
        $to_return .= '+1DAY-1MILLISECOND';
      }
      else {
        $to_return .= '/DAY';
      }
      break;

    case 'YYYY-MM-DDThh:mm:ssZ':
      if ($until) {
        $to_return .= '+1SECOND-1MILLISECOND';
      }
      else {
        $to_return .= '/SECOND';
      }
      break;

    default:
      // XXX: Maintain original behaviour of returning NULL... Should perhaps
      // throw an exception? Dunno.
      return;
  }

  return $to_return;
}

/**
 * Function that performs the queries to Solr.
 *
 * @param int $start
 *   The starting point for Solr queries, mainly used for resumption tokens.
 * @param string $field
 *   Field which we are using to query Solr.
 * @param string $query
 *   Query string to search for.
 * @param bool $set
 *   Whether we are querying down for a set which requires walking in ITQL land.
 * @param bool $date_query
 *   Whether this is a date query or not.
 *
 * @return array
 *   The results generated from the crafted Solr query.
 */
function islandora_oai_query_solr($start = 0, $field = 'PID', $query = NULL, $set = NULL, $date_query = NULL) {
  global $user;

  $query_processor = new IslandoraSolrQueryProcessor();

  // Build the query string.
  if (empty($query)) {
    $query = '*:*';
  }
  else {
    $query = $field . ':' . Apache_Solr_Service::escape($query);
  }

  $query_processor->buildQuery($query);
  $query_processor->solrParams['fl'] = '*, PID, ' . variable_get('islandora_oai_date_field', 'fgs_lastModifiedDate_dt');

  if ($date_query) {
    $query_processor->solrParams['fq'][] = $date_query;
  }

  $exclude_content_models = islandora_oai_get_restricted_models();
  $has_model = variable_get('islandora_oai_content_model_field', 'RELS_EXT_hasModel_uri_ms');
  foreach ($exclude_content_models as $content_model) {
    $content_model = trim($content_model);
    if ($content_model) {
      $query_processor->solrParams['fq'][] = '(-' . $has_model . ':("' . $content_model . '" OR "info:fedora/' . $content_model . '"))';
    }
  }
  if ($set) {
    // OAI uses : to separate multiple setSpecs so we had to replace it
    // from the pids earlier.
    $sets = explode(':', $set);
    $descendants = array();

    // Get all descendant children (collection and objs) and add to the sets
    // array if not already existing.
    // Cannot constrain on collections only as name conventions may change
    // across implementations.
    foreach ($sets as $key => $value) {
      // Replace the first occurrence of _ with a : so we are back to a
      // valid pid.
      $value = preg_replace('/_/', ':', $value, 1);
      $value = trim($value);
      $sets[$key] = $value;

      $itql_query = '
      select $child from <#ri>
      where
      (
        (
          (
          $child <fedora-model:hasModel> $model
          and $model <mulgara:is> <info:fedora/islandora:collectionCModel>
          and $parent_model <mulgara:is> <info:fedora/fedora-system:ContentModel-3.0>
          )
        or
          (
          $child <fedora-model:hasModel> $model
          and $model <fedora-model:hasModel> $parent_model
          and $parent_model <mulgara:is> <info:fedora/islandora:collectionCModel>
          and $child <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
          )
        minus $child <http://islandora.ca/ontology/relsext#isViewableByRole> $role
        minus $child <http://islandora.ca/ontology/relsext#isViewableByUser> $user
        )
      or
        (
          (
          $child <fedora-model:hasModel> $model
          and $model <mulgara:is> <info:fedora/islandora:collectionCModel>
          and $parent_model <mulgara:is> <info:fedora/fedora-system:ContentModel-3.0>
          )
        or
          (
          $child <fedora-model:hasModel> $model
          and $model <fedora-model:hasModel> $parent_model
          and $parent_model <mulgara:is> <info:fedora/islandora:collectionCModel>
          and $child <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
          )
      and
          (';
      foreach ($user->roles as $role) {
        $itql_query .= '$child <http://islandora.ca/ontology/relsext#isViewableByRole> ' . "'$role' or ";
      }
      if ($user->uid === 0) {
        $user_name = 'anonymous';
      }
      else {
        $user_name = $user->name;
      }
      $itql_query .= '$child <http://islandora.ca/ontology/relsext#isViewableByUser> ' . "'$user_name'" . ')';
      $itql_query .= ')
      )
    and
      (
      walk
        (
          $parent <fedora-rels-ext:isMemberOfCollection><info:fedora/' . $value . '>
           and $child <fedora-rels-ext:isMemberOfCollection> $parent
        )
      or
      walk
        (
          $parent <fedora-rels-ext:isMemberOf><info:fedora/' . $value . '>
          and $child <fedora-rels-ext:isMemberOf> $parent
        )
    )';

      $connection = islandora_get_tuque_connection();
      $query_results = $connection->repository->ri->itqlQuery($itql_query);
      foreach ($query_results as $result) {
        $walk_set = $result['child']['uri'];
        $walk_set = str_replace('info:fedora/', '', $walk_set);
        if (!in_array($walk_set, $descendants)) {
          $descendants[] = $walk_set;
        }
      }
      $descendants[] = $value;
    }
    $walked_sets = $descendants;

    $set_fq = array();
    // We are using OR here to account for fields in Solr that may index
    // just the PID or the entire URI. In the future if performance becomes
    // an issue with huge Solr queries we should re-visit this.
    foreach ($walked_sets as $walk) {
      foreach (islandora_oai_get_membership_array() as $collection_field) {
        $set_fq[] = $collection_field . ':("' . $walk . '" OR "info:fedora/' . $walk . '")';
      }
    }
    $query_processor->solrParams['fq'][] = '(' . implode(' OR ', $set_fq) . ')';
  }
  $query_processor->solrStart = $start;
  $query_processor->solrLimit = variable_get('islandora_oai_max_size', '20');
  try {
    $query_processor->executeQuery(FALSE);
    $solr_results = $query_processor->islandoraSolrResult['response'];
  }
  catch (Exception $e) {
    drupal_set_message(t('error searching @message', array('@message' => $e->getMessage())), 'error');
  }
  return $solr_results;
}


/**
 * Returns the collection fields defined in the admin page in array format.
 *
 * @return array
 *   An array containing the fields.
 */
function islandora_oai_get_membership_array() {
  // Store in a static variable, to avoid re-parsing within the same request.
  $mini_cache = &drupal_static(__FUNCTION__, array());

  if (empty($mini_cache)) {
    foreach (preg_split('/(\\r?\\n|\\r)+/', variable_get('islandora_oai_collection_field', "RELS_EXT_isMemberOfCollection_uri_ms\nRELS_EXT_isMemberOf_uri_ms")) as $spec) {
      $mini_cache[] = trim($spec);
    }
    $mini_cache = array_filter($mini_cache);
  }
  return $mini_cache;
}

/**
 * Get the restricted/excluded models.
 *
 * @return array
 *   An array of strings representing the models of which objects should not be
 *   returned from OAI requests.
 */
function islandora_oai_get_restricted_models() {
  $exclude_content_models = variable_get('islandora_oai_exclude_content_models', '');
  return explode("\n", $exclude_content_models);
}