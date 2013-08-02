BUILD STATUS
------------
Current build status:
[![Build Status](https://travis-ci.org/Islandora/islandora_oai.png?branch=7.x)](https://travis-ci.org/Islandora/islandora_oai)

CI Server:
http://jenkins.discoverygarden.ca

ISLANDORA OAI
==================

CONTENTS OF THIS FILE
---------------------

* summary
* functionality
* requirements
* installation
* configuration
* customization
* notes


SUMMARY
-------

Islandora OAI Provider Module

This module provides an implementation the Open Archives Initiative Protocol for Metadata Harvesting (OAI-PMH)
for Islandora fedora repository with a solr index. By implementing the islanodra_oa module,
you can expose content (its metadata) as an OAI-PMH repository. It will then be accessible by OAI harvesters
For further OAI documentation, please see here: http://www.openarchives.org/OAI/openarchivesprotocol.html


FUNCTIONALITY
--------------

Receive OAI-PMH request of service providers in form of HTTP request (GET or POST)
Handle the OAI-PMH request
Respond to OAI-PMH request in form of HTTP response (XML) to the service provider


REQUIREMENTS
------------

Drupal 7.x
Islandora
Islandora Solr


INSTALLATION
------------

Configure the basic details of the repository through /admin/islandora/islandora-oai

After you have exposed content types and some fields, your repository is available at /oai2

Some example requests are as follows:
 */oai2?verb=Identify
 */oai2?verb=ListMetadataFormats
 */oai2?verb=ListIdentifiers&metadataPrefix=oai_dc
 */oai2?verb=GetRecord&metadataPrefix=oai_dc&identifier=PID
 */oai2?verb=ListRecords&metadataPrefix=oai_dc

If the XACML module is present you will need to configure the rels.isViewableBy fields in the admin
page such that the OAI requests respect these object restrictions.


CUSTOMIZATION
-------------

By default the vanilla islandora_oai module provides a very basic output.

It is possible to add additional content to the description field of the repository.

This includes pointing at other harvesters and repositories, branding information etc.

An example of how to implement these can be referenced in the 6.x version of the module:

https://github.com/Islandora/islandora_oai/blob/6.x/islandora_oai.module#L534-L604.

NOTES
-----

The original 6.x version of this module was based off of the OAI2ForCCK module
located here: http://drupal.org/project/oai2forcck