<?php

/**
 * @file
 * Handles OAI requests and responds as appropriate.
 */

/**
 * Parses the URL and calls the appropriate function to handle the request.
 */
function islandora_oai_parse_request() {
  global $base_url;

  // Instantiate a new XML Writer object we will pass around.
  $writer = new XMLWriter();
  $writer->openURI('php://output');
  $writer->startDocument('1.0', 'utf-8');
  $writer->startElement('OAI-PMH');
  $writer->writeAttribute('xmlns', 'http://www.openarchives.org/OAI/2.0/');
  $writer->writeAttribute('xmlns:xsi', 'http://www.w3.org/2001/XMLSchema-instance');
  $writer->writeAttribute('xsi:schemaLocation', 'http://www.openarchives.org/OAI/2.0/ http://www.openarchives.org/OAI/2.0/OAI-PMH.xsd');
  $response_date = gmstrftime('%Y-%m-%dT%H:%M:%SZ');
  $writer->writeElement('responseDate', $response_date);

  // Arguments are all GET arguments except for Drupal's q.
  if (isset($_GET) && count($_GET) != 1) {
    $args = $_GET;
    $verb = $_GET['verb'];
    unset($args['q']);
    unset($args['verb']);
  }
  // Allow post requests.
  elseif (isset($_POST) && count($_POST) != 0) {
    $args = $_POST;
    $verb = $_POST['verb'];
    unset($args['verb']);
  }
  // Error if no GET arguments.
  else {
    islandora_oai_add_error('badRequestMethod', $_SERVER['REQUEST_METHOD']);
  }

  // Argument repeated more than once in the GET request.
  $arg_list = explode('&', ($_SERVER['QUERY_STRING']));
  $arg_list = (array_count_values($arg_list));

  $repeat_arg = FALSE;

  // Repeating arguments within the request.
  foreach ($arg_list as $value) {
    if ($value > 1) {
      islandora_oai_add_error('sameArgument');
      $repeat_arg = TRUE;
      break;
    }
  }

  // Set request URL.
  if (isset($_GET['q'])) {
    $request_url = $base_url . '/' . $_GET['q'];
  }
  else {
    $request_url = $base_url;
  }

  header('Content-Type: text/xml');
  // Add the request.
  $writer->startElement('request');

  // Check to see if we have a bad verb.
  $error_list = islandora_oai_get_errors();

  if (!count($error_list)) {
    $writer->writeAttribute('verb', $verb);
  }
  $writer->text($request_url);
  $writer->endElement();

  // Switch on verb and dispatch requests.
  if (!$repeat_arg) {
    switch ($verb) {

      case 'GetRecord':
        islandora_oai_get_record($writer, $args);
        break;

      case 'Identify':
        islandora_oai_identify($writer, $args);
        break;

      case 'ListIdentifiers':
        islandora_oai_list_id_rec($writer, $args);
        break;

      case 'ListMetadataFormats':
        islandora_oai_list_metadata_formats($writer, $args);
        break;

      case 'ListRecords':
        islandora_oai_list_id_rec($writer, $args, TRUE);
        break;

      case 'ListSets':
        islandora_oai_list_sets($writer, $args);
        break;

      case '':
        islandora_oai_add_error('noVerb');
        break;

      default:
        islandora_oai_add_error('badVerb', $verb);
    }
  }

  // Grab errors again for output.
  $error_list = islandora_oai_get_errors();

  // Get all errors and add them to the response.
  if ($error_list) {
    foreach ($error_list as $code => $verb) {
      $writer->startElement('error');
      $writer->writeAttribute('code', $code);
      $writer->text($verb);
      $writer->endElement();
    }
  }
  $writer->endDocument();
  $writer->flush();
}

/**
 * Returns errors generated during an OAI request.
 *
 * @return bool|array
 *   FALSE when no errors, an array containing all errors encountered otherwise.
 */
function islandora_oai_get_errors() {
  // Get the errors array and convert it to a string.
  $errors = islandora_oai_add_error('get');

  // Return false if no errors.
  if (!count($errors)) {
    return FALSE;
  }
  return $errors;
}

/**
 * Routs a request to add an error to the error function in a sensical way.
 *
 * @param string $code
 *   Error code to be added, or 'get' if we want to get the errors array.
 * @param string $argument
 *   The argument causing the error.
 * @param string $value
 *   The argument value causing the error.
 *
 * @return NULL|array
 *   Nothing is returned if not 'get', an array of errors otherwise.
 */
function islandora_oai_add_error($code, $argument = '', $value = '') {
  return islandora_oai_errors($code, $argument, $value);
}

/**
 * Define errors based on OAI2 specification.
 *
 * Helper function for islandora_oai_add_error and islandora_oai_get_errors.
 * Basically faking a singleton with getter and setter methods.
 * OAI2 specification: http://www.openarchives.org/OAI/openarchivesprotocol.html
 *
 * @param string $code
 *   Error code to be added, or 'get' if we want to get the errors array.
 * @param string $argument
 *   The argument causing the error.
 * @param string $value
 *   The argument value causing the error.
 *
 * @return NULL|array
 *   Nothing is returned if not 'get', an array of errors otherwise.
 */
function islandora_oai_errors($code, $argument = '', $value = '') {
  // Store errors in a static array.
  static $errors;

  // Initialize $errors.
  if (!is_array($errors)) {
    $errors = array();
  }

  switch ($code) {

    // Return errors if this is a get errors request.
    case 'get':
      return $errors;

    // Clear out errors and return nothing.
    case 'reset':
      $errors = array();
      return;

    case 'badArgument':
      $text = t("The argument '@argument' (value=@value) included in the request is not valid.", array('@argument' => $argument, '@value' => $value));
      break;

    case 'badGranularity':
      $text = t("The value '@value' of the argument '@argument' is not valid.", array('@value' => $value, '@argument' => $argument));
      $code = 'badArgument';
      break;

    case 'badResumptionToken':
      $text = t("The resumptionToken '@value' does not exist or has already expired.", array('@value' => $value));
      break;

    case 'badRequestMethod':
      $text = t("The request method '@argument' is unknown.", array('@argument' => $argument));
      $code = 'badVerb';
      break;

    case 'badVerb':
      $text = t("The verb '@argument' provided in the request is illegal.", array('@argument' => $argument));
      break;

    case 'cannotDisseminateFormat':
      $text = t("The metadata format '@value' given by @argument is not supported by this repository.", array('@value' => $value, '@argument' => $argument));
      break;

    case 'exclusiveArgument':
      $text = t('The usage of resumptionToken as an argument allows no other arguments.');
      $code = 'badArgument';
      break;

    case 'idDoesNotExist':
      $text = t("The value '@value' of the identifier is illegal for this repository.", array('@value' => $value));
      break;

    case 'missingArgument':
      $text = t("The required argument '@argument' is missing in the request.", array('@argument' => $argument));
      $code = 'badArgument';
      break;

    case 'noRecordsMatch':
      $text = t('The combination of the given values results in an empty list.');
      break;

    case 'noMetadataFormats':
      $text = t('There are no metadata formats available for the specified item.');
      break;

    case 'noVerb':
      $text = t('The request does not provide any verb.');
      $code = 'badVerb';
      break;

    case 'noSetHierarchy':
      $text = t('This repository does not support sets.');
      break;

    case 'sameArgument':
      $text = t('Do not use the same argument more than once.');
      $code = 'badArgument';
      break;

    case 'sameVerb':
      $text = t('Do not use the same verb more than once.');
      $code = 'badVerb';
      break;

    default:
      $text = t("Unknown error:
        code: '@code',
        argument: '@argument',
        value: '@value'",
        array('@code' => $code, '@argument' => $argument, '@value' => $value));
      $code = 'badArgument';
  }
  $errors[$code] = $text;
}

/**
 * Retrieve an individual metadata record from a repository.
 *
 * @param string $writer
 *   The XMLWriter object being passed by reference for constructing our output.
 * @param array $args
 *   Argument of the $_POST or $_GET request.
 *
 * @return bool
 *   FALSE if we encountered an error.
 */
function islandora_oai_get_record(&$writer, $args) {
  foreach ($args as $key => $val) {
    switch ($key) {
      case 'identifier':
        // Replace the first _ with a : so we have a pid.
        $val = preg_replace('/_/', ':', $val, 1);
        // Strip off the the prefix such that we are left with a Fedora pid.
        $id_prefix = trim(variable_get('islandora_oai_repository_identifier', 'drupal-site.org'));
        $identifier = str_replace('oai:' . $id_prefix . ':', '', $val);
        break;

      case 'metadataPrefix':
        $result = db_query("SELECT * FROM {islandora_oai_metadata_formats} WHERE metadata_prefix=:metadata_prefix", array(':metadata_prefix' => $val));
        if ($result->rowCount() > 0) {
          $metadata_prefix = $val;
        }
        else {
          islandora_oai_add_error('cannotDisseminateFormat', $key, $val);
          return FALSE;
        }
        break;

      default:
        islandora_oai_add_error('badArgument', $key, $val);
        return FALSE;
    }
  }

  if (!isset($identifier)) {
    islandora_oai_add_error('missingArgument', 'identifier');
    return FALSE;
  }
  if (!isset($metadata_prefix)) {
    islandora_oai_add_error('missingArgument', 'metadataPrefix');
    return FALSE;
  }

  $results = islandora_oai_query_solr(0, 'PID', $identifier);
  $record = $results['objects'][0];

  if (!$record['PID']) {
    islandora_oai_add_error('idDoesNotExist', '', $identifier);
    return FALSE;
  }

  $writer->startElement('GetRecord');
  islandora_oai_build_record_response($writer, $record, TRUE, $metadata_prefix);
  $writer->endElement();
}

/**
 * Get the date granularity.
 *
 * @param string $date
 *   The date string to match against.
 *
 * @return string|bool
 *   The granularity of the date, or FALSE.
 */
function islandora_oai_get_date_granularity($date) {
  if (preg_match("/^\d\d\d\d\-\d\d\-\d\d$/", $date)) {
    return 'YYYY-MM-DD';
  }
  elseif (preg_match("/^(\d\d\d\d\-\d\d\-\d\d)T(\d\d:\d\d:\d\d)Z$/", $date)) {
    return 'YYYY-MM-DDThh:mm:ssZ';
  }
  else {
    return FALSE;
  }
}

/**
 * Validate the date format (from or until in the request).
 *
 * @param string $date
 *   The date passed through the request.
 *
 * @return bool
 *   TRUE if the date format is valid, FALSE otherwise.
 */
function islandora_oai_check_date_format($date) {
  static $granularity;
  $new_granularity = islandora_oai_get_date_granularity($date);

  if (!$new_granularity) {
    return FALSE;
  }

  if ($granularity) {
    if ($granularity != $new_granularity) {
      return FALSE;
    }
  }
  else {
    $granularity = $new_granularity;
  }

  // Validate date based on granularity.
  if ($granularity == 'YYYY-MM-DDThh:mm:ssZ') {
    $checkstr = '([0-9]{4})-([0-9]{1,2})-([0-9]{1,2})T([0-9]{2}):([0-9]{2}):([0-9]{2})Z';
  }
  else {
    $checkstr = '([0-9]{4})-([0-9]{1,2})-([0-9]{1,2})';
  }

  if (preg_match("/$checkstr/", $date, $regs)) {
    if (checkdate($regs[2], $regs[3], $regs[1])) {
      return TRUE;
    }
    else {
      return FALSE;
    }
  }
  else {
    return FALSE;
  }
}

/**
 * Generate a random value for resumptionToken.
 *
 * @return int
 *   A random number to be used for a resumption token ID.
 */
function islandora_oai_get_random_token() {
  return mt_rand(12345, mt_getrandmax());
}

/**
 * Provides the repository identity information.
 *
 * @param object $writer
 *   The XMLWriter object being passed by reference for constructing our output.
 * @param array $args
 *   Argument of the $_POST or $_GET request.
 *
 * @return bool
 *   FALSE if an error is encoutered.
 */
function islandora_oai_identify(&$writer, $args) {
  $error_text = islandora_oai_get_errors();

  // Catch for a bad argument.
  if (count($error_text) > 0 && (count($args) > 0)) {
    foreach ($args as $key => $val) {
      islandora_oai_add_error('badArgument', $key, $val);
      return FALSE;
    }
  }

  $earliest_datestamp = islandora_oai_get_earliest_datetime();
  $writer->startElement('Identify');
  $writer->writeElement('repositoryName', variable_get('islandora_oai_repository_name', 'Repository'));
  $writer->writeElement('baseURL', variable_get('islandora_oai_path', 'oai2'));
  $writer->writeElement('protocolVersion', '2.0');
  $writer->writeElement('adminEmail', variable_get('islandora_oai_admin_email', 'N/A'));
  $writer->writeElement('earliestDatestamp', $earliest_datestamp);
  $writer->writeElement('deletedRecord', 'no');
  $writer->writeElement('granularity', 'YYYY-MM-DDThh:mm:ssZ');
  $writer->writeElement('compression', 'yes');

  // Description.
  $writer->startElement('description');

  // OAI Identifer.
  $writer->startElement('oai-identifier');
  $writer->writeAttribute('xmlns', 'http://www.openarchives.org/OAI/2.0/oai-identifier');
  $writer->writeAttribute('xmlns:xsi', 'http://www.w3.org/2001/XMLSchema-instance');
  $writer->writeAttribute('xsi:schemaLocation', 'http://www.openarchives.org/OAI/2.0/oai-identifier
    http://www.openarchives.org/OAI/2.0/oai-identifier.xsd');
  $writer->writeElement('scheme', 'oai');
  $writer->writeElement('repositoryIdentifier', trim(variable_get('islandora_oai_repository_identifier', 'drupal-site.org')));
  $writer->writeElement('delimiter', ':');
  $writer->writeElement('sampleIdentifier', 'oai:' . trim(variable_get('islandora_oai_repository_identifier', 'drupal-site.org') . ':123'));
  $writer->endElement();
  $writer->endElement();
}

/**
 * Retrieve the earliest datetime of content using Fedora's RI.
 *
 * @return string
 *   The earliest datestamp retrieved from Fedora.
 */
function islandora_oai_get_earliest_datetime() {
  static $earliest_datestamp;

  if (!$earliest_datestamp) {
    $object = islandora_object_load(variable_get('islandora_repository_pid', 'islandora:root'));

    $itql_query = 'select $object $date from <#ri>
                             where $object <info:fedora/fedora-system:def/model#label> $title
                             and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
                             and $object <info:fedora/fedora-system:def/view#lastModifiedDate> $date
                             order by $date asc';

    $query_results = $object->repository->ri->itqlQuery($itql_query);
    $result = $query_results[0];
    $earliest_datestamp = $result['date']['value'];
    $earliest_datestamp = drupal_substr($earliest_datestamp, 0, -5) . 'Z';
  }
  return $earliest_datestamp;
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
    foreach (preg_split('/(\\r?\\n|\\r)+/', variable_get('islandora_oai_collection_field', 'RELS_EXT_isMemberOfCollection_uri_ms\nRELS_EXT_isMemberOf_uri_ms')) as $spec) {
      $mini_cache[] = trim($spec);
    }
    $mini_cache = array_filter($mini_cache);
  }
  return $mini_cache;
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

  $exclude_content_models = variable_get('islandora_oai_exclude_content_models', '');
  $exclude_content_models = explode("\n", $exclude_content_models);
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
      $itql_query .= '$child <http://islandora.ca/ontology/relsext#isViewableByUser> ' . "'$user->name'" . ')';
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
    }
    $walked_sets = $descendants;

    $set = str_replace('_', ':', $set);
    $walked_sets[] = $set;
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
 * Lists identifiers or records depending on values.
 *
 * @param object $writer
 *   The XMLWriter object being passed by reference for constructing our output.
 * @param array $args
 *   Arguments of the POST or GET Request
 * @param bool $list_rec
 *   List identifiers if FALSE, records if TRUE.
 *
 * @return bool
 *   FALSE if an error is encountered.
 */
function islandora_oai_list_id_rec(&$writer, $args, $list_rec = FALSE) {
  $query_args = array();
  // Max records to send in one response.
  $max_records = variable_get('islandora_oai_max_size', '20');
  $expire_seconds = variable_get('islandora_oai_expire_time', '86400');
  $expire_time = gmstrftime('%Y-%m-%dT%TZ', time() + $expire_seconds);

  // Handle error and set variables.
  foreach ($args as $key => $val) {
    switch ($key) {
      case 'from':
        if (!isset($from)) {
          $from = $val;
        }
        else {
          islandora_oai_add_error('badArgument', $key, $val);
          return FALSE;
        }
        // Ensure valid date/time format.
        if (islandora_oai_check_date_format($from)) {
          $from_time = islandora_oai_datetime_to_time($from);
        }
        else {
          islandora_oai_add_error('badGranularity', 'from', $from);
          return FALSE;
        }
        break;

      case 'until':
        if (!isset($until)) {
          $until = $val;
        }
        else {
          islandora_oai_add_error('badArgument', $key, $val);
          return FALSE;
        }
        // Ensure valid date/time format.
        if (islandora_oai_check_date_format($until)) {
          $until_time = islandora_oai_datetime_to_time($until);
        }
        else {
          islandora_oai_add_error('badGranularity', 'until', $until);
          return FALSE;
        }
        break;

      case 'set':
        if (!isset($set)) {
          $set = $val;
        }
        else {
          islandora_oai_error('badArgument', $key, $val);
          return FALSE;
        }
        break;

      case 'metadataPrefix':
        if (!isset($metadata_prefix)) {
          $result = db_query("SELECT * FROM {islandora_oai_metadata_formats} WHERE metadata_prefix=:metadata_prefix ", array(':metadata_prefix' => $val));

          if ($result->rowCount() > 0 && !isset($metadata_prefix)) {
            $metadata_prefix = $val;
          }
          else {
            islandora_oai_add_error('cannotDisseminateFormat', $key, $val);
            return FALSE;
          }
        }
        else {
          islandora_oai_add_error('badArgument', $key, $val);
          return FALSE;
        }
        break;

      case 'resumptionToken':
        // If a resumptionToken is present, we cannot have any other arguments.
        if (count($args) > 1) {
          islandora_oai_errors('clear');
          islandora_oai_add_error('exclusiveArgument');
          return FALSE;
        }
        elseif (!isset($resumption_token)) {
          $resumption_token = $val;
        }
        else {
          islandora_oai_add_error('badArgument', $key, $val);
          return FALSE;
        }
        break;

      default:
        islandora_oai_add_error('badArgument', $key, $val);
        return FALSE;
    }
  }

  // Resume previous session?
  if (isset($resumption_token)) {
    $token = islandora_oai_get_token($resumption_token);
    $metadata_prefix = $token->metadata_prefix;
    if (!is_object($token)) {
      return FALSE;
    }
    elseif ($token->timestamp < (time() - variable_get('islandora_oai_expire_time', '86400'))) {
      islandora_oai_add_error('badResumptionToken', '', $resumption_token);
      return FALSE;
    }
    $query_args = explode('#', $token->queryargs);
    $set = $token->setargs;
    if (isset($query_args) && (!empty($query_args[0]) || !empty($query_args[1]))) {
      $from = $query_args[0];
      $until = $query_args[1];
    }
  }
  // Make a new session.
  else {
    $token = new stdClass();
    $token->deliveredrecords = 0;
    $token->extquery = '';

    // Error checking.
    if (!isset($metadata_prefix)) {
      islandora_oai_add_error('missingArgument', 'metadataPrefix');
      return FALSE;
    }

    if (isset($from) && isset($until)) {
      if ($from_time > $until_time) {
        islandora_oai_add_error('badArgument', 'until and from', $from . ' and ' . $until);
        return FALSE;
      }
    }
  }

  $field = 'PID';
  $query = NULL;
  $date_query = NULL;

  if (isset($from) || isset($until)) {
    $field = variable_get('islandora_oai_date_field', 'fgs_lastModifiedDate_dt');

    if (isset($from)) {
      $solr_from = islandora_oai_datetime_to_solr_time($from);
    }
    else {
      $solr_from = '*';
    }

    if (isset($until)) {
      $solr_until = islandora_oai_datetime_to_solr_time($until);
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
  $num_rows = $results['numFound'];

  if ($num_rows) {
    if ($list_rec) {
      $writer->startElement('ListRecords');
    }
    else {
      $writer->startElement('ListIdentifiers');
    }
    foreach ($results['objects'] as $doc) {
      islandora_oai_build_record_response($writer, $doc, $list_rec, $metadata_prefix);
    }

    if ($num_rows - $token->deliveredrecords > $max_records) {
      if (!isset($metadata_prefix)) {
        $metadata_prefix = 'oai_dc';
      }
      $token_string = islandora_oai_get_random_token();
      $arg_string = implode("#", $query_args);
      $delivered_records = (int) $token->deliveredrecords + $max_records;

      db_insert('islandora_oai_tokens')
        ->fields(array(
          'id' => $token_string,
          'deliveredrecords' => $delivered_records,
          'queryargs' => isset($arg_string) ? $arg_string : NULL,
          'setargs' => isset($set) ? $set : NULL,
          'metadata_prefix' => $metadata_prefix,
          'timestamp' => time(),
        ))
        ->execute();

      islandora_oai_write_token_xml($writer, $num_rows, $token->deliveredrecords, $expire_time, $token_string);
    }
    // Last delivery, return empty ResumptionToken.
    elseif (isset($resumption_token)) {
      islandora_oai_write_token_xml($writer, $num_rows, $token->deliveredrecords);
    }
    $writer->endElement();
  }
  else {
    islandora_oai_add_error('noRecordsMatch');
    return FALSE;
  }
}

/**
 * Build a record response given a Solr response doc.
 *
 * @param object $writer
 *   The XMLWriter object being passed by reference for constructing our output.
 * @param array $record
 *   Solr response document.
 * @param bool $full_record
 *   Whether to display one record or not.
 * @param string $metadata_prefix
 *   The metadata prefix we are searching for.
 */
function islandora_oai_build_record_response(&$writer, $record, $full_record = FALSE, $metadata_prefix = 'oai_dc') {
  $identifier = $record['solr_doc']['PID'];
  $identifier = str_replace(':', '_', $identifier);
  $repo_id = variable_get('islandora_oai_repository_identifier', 'drupal-site.org');
  $date = new DateTime((string) $record['solr_doc'][variable_get('islandora_oai_date_field', 'fgs_lastModifiedDate_dt')], new DateTimeZone('UTC'));
  $date_stamp = $date->format("Y-m-d\TH:i:s\Z");

  if ($full_record) {
    $writer->startElement('record');
  }

  $writer->startElement('header');
  $writer->writeElement('identifier', 'oai:' . $repo_id . ':' . $identifier);
  $writer->writeElement('datestamp', $date_stamp);

  foreach (islandora_oai_get_membership_array() as $collection_field) {
    if (isset($record[$collection_field])) {
      // Need to cast to array such that we can check for multiple collections.
      foreach ((array) $record[$collection_field] as $set) {
        $set = str_replace('info:fedora/', '', $set);
        $set_spec = str_replace(':', '_', $set);
        $writer->writeElement('setSpec', $set_spec);
      }
    }
  }

  $writer->endElement();

  // List record values.
  if ($full_record) {
    $object = islandora_object_load($record['PID']);
    $param = variable_get("islandora_oai_metadata_datastream_id_$metadata_prefix", 'DC');

    if (isset($object[$param])) {
      $content = $object->repository->api->a->getDatastreamDissemination($object->id, $param);
    }
    else {
      $content = NULL;
    }

    $oai_output = NULL;

    if (!empty($content)) {
      $xslt = new XSLTProcessor();
      $xsl = new DOMDocument();
      $file = variable_get("islandora_oai_transform_file_$metadata_prefix", 'default');
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
          '@pidname' => $record['PID'],
          '@file' => $file,
          '@datastream' => $param,
        ), WATCHDOG_ERROR);
    }
    // Datastream doesn't exist.
    elseif (!isset($object[$param])) {
      watchdog('islandora_oai', "The pid @pidname failed to transform on the @datastream datastream as the datastream doesn't exist.",
        array(
          '@pidname' => $record['PID'],
          '@datastream' => $param,
        ), WATCHDOG_ERROR);
    }

    if (!empty($oai_output)) {
      $writer->startElement('metadata');
      $writer->writeRaw($oai_output);
      $writer->endElement();
    }
    // Close our record.
    $writer->endElement();
  }
}

/**
 * Converts a time string to a DateTime string.
 *
 * @param string $datetime
 *   Time string pulled from the request.
 *
 * @return DateTime
 *   A DateTime string.
 */
function islandora_oai_datetime_to_time($datetime) {
  $time = strtotime($datetime);
  return $time;
}

/**
 * Converts a time string into a form Solr can parse.
 *
 * @param string $datetime
 *   Time string to be parsed.
 *
 * @return string
 *   A string representing the time in the given granularity.
 */
function islandora_oai_datetime_to_solr_time($datetime) {
  $granularity = islandora_oai_get_date_granularity($datetime);

  switch ($granularity) {
    case 'YYYY-MM-DD':
      return $datetime . 'T00:00:00Z/DAY';

    case 'YYYY-MM-DDThh:mm:ssZ':
      return $datetime;

  }
}

/**
 * Generate the resumption token given a token string.
 *
 * @param object $writer
 *   The XMLWriter object being passed by reference for constructing our output.
 * @param int $num_rows
 *   The complete list size.
 * @param int $delivered_records
 *   The number of records that have been delivered.
 * @param string $expire_time
 *   The expiration time of the resumption token.
 * @param string $token
 *   The ID of the resumption token.
 */
function islandora_oai_write_token_xml(&$writer, $num_rows, $delivered_records, $expire_time = NULL, $token = NULL) {
  if ($token) {
    $writer->startElement('resumptionToken');
    $writer->writeAttribute('expirationDate', $expire_time);
    $writer->writeAttribute('completeListSize', $num_rows);
    $writer->writeAttribute('cursor', $delivered_records);
    $writer->text($token);
    $writer->endElement();
  }
  // Last delivery, return empty ResumptionToken.
  else {
    $writer->startElement('resumptionToken');
    $writer->writeAttribute('completeListSize', $num_rows);
    $writer->writeAttribute('cursor', $delivered_records);
    $writer->endElement();
  }
}

/**
 * Retrieves a resumption token from the db.
 *
 * @param string $resumption_token
 *   The ID of the resumption token we are searching for.
 *
 * @return bool|stdObject
 *   FALSE if the token doesn't exist, the token object otherwise.
 */
function islandora_oai_get_token($resumption_token) {

  $token_query = db_query("SELECT * FROM {islandora_oai_tokens} WHERE id= :resumptiontoken LIMIT 0,1", array(':resumptiontoken' => $resumption_token))->fetchObject();;

  if ($token_query && $token_query->deliveredrecords) {
    return $token_query;
  }
  else {
    islandora_oai_add_error('badResumptionToken', '', $resumption_token);
    return FALSE;
  }
}

/**
 * List all the Metadata formats of this repository.
 *
 * @param object $writer
 *   The XMLWriter object being passed by reference for constructing our output.
 * @param array $args
 *   Arguments of the $_GET or $_POST request.
 *
 * @return bool
 *   FALSE if an error is encountered.
 */
function islandora_oai_list_metadata_formats(&$writer, $args) {
  foreach ($args as $key => $val) {
    switch ($key) {
      case 'identifier':
        $repo_id = trim(variable_get('islandora_oai_repository_identifier', 'drupal-site.org'));
        $identifier = str_replace('oai:' . $repo_id . ':', '', $val);

        $pid_query = str_replace('_', ':', $identifier);
        $pid_query = 'info:fedora/' . $pid_query;
        $itql_query = 'select $object $title from <#ri>
        where $object <fedora-model:label> $title and $object <mulgara:is> <' . $pid_query . '>';

        $connection = islandora_get_tuque_connection();
        $query_results = $connection->repository->ri->itqlQuery($itql_query);

        foreach ($query_results as $result) {
          $result_pid = $result['object']['uri'];
        }

        if (!$result_pid == $pid_query) {
          islandora_oai_add_error('idDoesNotExist', 'identifier', $identifier);
          return FALSE;
        }

        break;

      case 'metadataPrefix':
        $result = db_query("SELECT * FROM {islandora_oai_metadata_formats} WHERE metadata_prefix=:metadata_prefix", array(':metadata_prefix' => $val));
        if ($result->rowCount() > 0) {
          $metadata_prefix = $val;
        }
        else {
          islandora_oai_add_error('cannotDisseminateFormat', $key, $val);
          return FALSE;
        }

        break;

      default:
        islandora_oai_add_error('badArgument', $key, $val);
        return FALSE;
    }
  }

  $list_meta_data_formats = array();
  $result = db_query("SELECT * FROM {islandora_oai_metadata_formats}");

  foreach ($result as $fmt) {
    $list_meta_data_formats[] = array(
      'metadataPrefix' => $fmt->metadata_prefix,
      'schema' => $fmt->oai2_schema,
      'metadataNamespace' => $fmt->metadata_namespace,
    );
  }

  if (count($list_meta_data_formats) > 0) {
    $writer->startElement('ListMetadataFormats');
    foreach ($list_meta_data_formats as $entry) {
      $writer->startElement('metadataFormat');
      $writer->writeElement('metadataPrefix', $entry['metadataPrefix']);
      $writer->writeElement('schema', $entry['schema']);
      $writer->writeElement('metadataNamespace', $entry['metadataNamespace']);
      $writer->endElement();
    }
    $writer->endElement();
  }
  else {
    islandora_oai_add_error('noMetadataFormats');
  }
}

/**
 * ListSets of this repository implemented as a list of collections.
 *
 * @param object $writer
 *   The XMLWriter object being passed by reference for constructing our output.
 *
 * @param array $args
 *   Arguments of the $_POST or the $_GET request.
 *
 * @return bool
 *   FALSE if an error is encountered.
 */
function islandora_oai_list_sets($writer, $args) {
  module_load_include('inc', 'islandora', 'includes/utilities');

  // Max records to send in one response.
  $max_records = variable_get('islandora_oai_max_size', '20');
  $expire_seconds = variable_get('islandora_oai_expire_time', '86400');
  $expire_time = gmstrftime('%Y-%m-%dT%TZ', time() + $expire_seconds);
  // @TODO: Add support for descriptive metadata for sets?
  // http://www.openarchives.org/OAI/openarchivesprotocol.html#ListSets.
  foreach ($args as $key => $val) {
    switch ($key) {
      case 'resumptionToken':
        // If a resumptionToken is present, we cannot have any other arguments.
        if (count($args) > 1) {
          islandora_oai_errors('clear');
          islandora_oai_add_error('exclusiveArgument');
          return FALSE;
        }
        elseif (!isset($resumption_token)) {
          $resumption_token = $val;
        }
        break;

      default:
        islandora_oai_add_error('badArgument', $key, $val);
        return FALSE;
    }
  }
  // Resume previous session?
  if (isset($resumption_token)) {
    $token = islandora_oai_get_token($resumption_token);
    if (!is_object($token)) {
      return FALSE;
    }
    elseif ($token->timestamp < (time() - $expire_seconds)) {
      islandora_oai_add_error('badResumptionToken', '', $resumption_token);
      return FALSE;
    }
  }
  // Make a new session.
  else {
    $token = new stdClass();
    $token->deliveredrecords = 0;
    $token->extquery = '';
  }
  $sparql_query = <<<EOQ
SELECT ?object ?title ?created FROM <#ri>
WHERE
{
  {
    ?object <fedora-model:hasModel> <info:fedora/islandora:collectionCModel>
  }
  UNION
  {
    ?object <fedora-model:hasModel> [<fedora-model:hasModel> <info:fedora/islandora:collectionCModel>]
  }
  ?object <fedora-model:label> ?title ;
          <fedora-model:createdDate> ?created

EOQ;
  $xacml_filters = array();
  if (module_exists('islandora_xacml_api')) {
    $xacml_statements = islandora_xacml_api_islandora_basic_collection_get_query_statements('view');
    $sparql_query .= implode(' . ', $xacml_statements);
    $xacml_filters = islandora_xacml_api_islandora_basic_collection_get_query_filters('view');
  }
  $filters = array();
  $namespace_enforced = FALSE;
  $namespace_enforced = islandora_islandora_basic_collection_get_query_filters();
  if ($namespace_enforced) {
    $filters[] = "FILTER($namespace_enforced)";
  }
  if (!empty($xacml_filters)) {
    $filters[] = "FILTER($xacml_filters)";
  }
  $sparql_query .= implode(' ', $filters);

  // Add on the ORDER BY and last } to complete the query.
  $sparql_query .= <<<EOQ
}
  ORDER BY ?title
EOQ;
  $connection = islandora_get_tuque_connection();
  $total_records = $connection->repository->ri->countQuery($sparql_query, 'sparql');
  $sparql_query .= "
  LIMIT $max_records
  OFFSET {$token->deliveredrecords}";
  $query_results = $connection->repository->ri->sparqlQuery($sparql_query);
  $print_list_set = FALSE;

  foreach ($query_results as $result) {
    // Logic to print our ListSets only if we have valid records. Will only fire
    // once.
    if (!$print_list_set) {
      $writer->startElement('ListSets');
      $print_list_set = TRUE;
    }
    $writer->startElement('set');
    $set_name = $result['title']['value'];

    if (empty($set_name)) {
      $set_name = t('Empty label');
    }
    $set_pid = str_replace(':', '_', $result['object']['value']);
    $writer->writeElement('setSpec', $set_pid);
    $writer->writeElement('setName', $set_name);
    $writer->endElement();
  }
  // Need to close our ListSets element.
  if ($print_list_set) {
    $writer->endElement();

    if ($total_records - $token->deliveredrecords > $max_records) {
      $token_string = islandora_oai_get_random_token();
      $delivered_records = (int) $token->deliveredrecords + $max_records;

      db_insert('islandora_oai_tokens')
        ->fields(array(
          'id' => $token_string,
          'deliveredrecords' => $delivered_records,
          'timestamp' => time(),
        ))
        ->execute();
      islandora_oai_write_token_xml($writer, $total_records, $token->deliveredrecords, $expire_time, $token_string);
    }
    // Last delivery, return empty ResumptionToken.
    elseif (isset($resumption_token)) {
      islandora_oai_write_token_xml($writer, $total_records, $token->deliveredrecords);
    }
    $writer->endElement();
  }
  else {
    islandora_oai_add_error('noSetHierarchy');
  }
}
