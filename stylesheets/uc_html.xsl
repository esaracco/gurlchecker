<?xml version="1.0" encoding="UTF-8"?>
<!-- STYLESHEET PARAMETERS

	* withlabel (1|0) [numeric]
	  -> 1 if links labels must be displayed
	
	* withnum (1|0) [numeric]
	  -> 1 if lists must be numbered

	* date [string]
	  -> date to display
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="html" version="1.0" encoding="utf-8" indent="no"/>
	
<xsl:template match="project">
	<html>
	<head>
	<style>
	  * {
                font-family: Verdana, Arial, Helvetica, sans-serif;
                font-size: 10px;
	  }

	  body {
		background: #C9D1E1;
	  }

	  a {
		color: #3B5990;
		text-decoration: none;
	  }

	  a:hover {
		text-decoration: underline;
	  }

          ol, ul {
                font-weight: bold;
          }

	  ol li, ul li {
		margin: 7px;
	  }

          h2, #footer {
                text-align: center;
                background: white;
                border: solid 1px #3B5990;
                color: #3B5990;
		padding: 5px;
          }

	  h2 {
		font-size: 2em;
	  }

	  h2 a {
		font-size: 1em;
	  }

	  #footer {
		font-style: italic;
	  }

	  #footer span {
		font-weight: bold;
	  }

	  .user-action-section, .virname-section {
		padding: 2px;
		margin-top: 5px;
		margin-left: 100px;
	  }

	  .user-action-section .todo, .virname-section .warning {
		background: white;
		font-weight: bold;
		padding: 2px;
	  }

	  .user-action-section .todo {
		color: blue;
	  }

	  .user-action-section .action, .virname-section .virname {
		font-weight: normal;
	  }

	  .virname-section .warning {
		color: red;
	  }

          table {
                width: 50em;
                background: #3B5990;
                border: solid 2px #729FCF;
                color: white;
		margin-left: auto;
		margin-right: auto;
          }

	  table td {
		vertical-align: top;
	  }

	  table td span {
		font-weight: bold;
		font-size: 1em;
	  }

	  table td a {
		color: white;
	  }

          .label {
                color: #C9D1E1;
                font-weight: bold;
                text-align: right;
          }

          .status_code {
                color: white;
                border: solid 1px #3B5990;
                border-width: 1px 1px 1px 5px;
		margin-right: 1em;
		padding: 2px;
          }

	  .good {
		background: green;
	  }

	  .bad {
		background: red;
	  }

	  .timeout {
		background: purple;
	  }

	  .ignored {
		background: pink;
	  }

	  .malformed {
		background: brown;
	  }

	  .restricted {
		background: blue;
	  }
	</style>
	</head>
	<body>
	  <h2><a href="http://gurlchecker.labs.libre-entreprise.org" target="_BLANK">gURLChecker</a> Project Report</h2>
	  
	  <p>
	  <table>
	    <xsl:variable name="url" select="website/@url"/>
	    <tr><td class="label">Main project URL :</td><td><a href="{$url}" target="_BLANK"><xsl:value-of select="$url"/></a></td></tr>
	  </table>
	  </p>
		
	  <xsl:apply-templates select="report/links"/>

          <div id="main"><xsl:apply-templates select="website"/></div>

	  <div id="footer">Report generated on <span><xsl:value-of select="$date"/></span></div>

	</body>
	</html>
</xsl:template>

<xsl:template match="report/links">
	<p>
	<table>
	  <tr><td class="label" width="50%">Total :</td><td width="50%"><span><xsl:value-of select="@all"/></span> links discovered</td></tr>
	  <tr><td class="label" width="50%">Checked :</td><td width="50%"><span><xsl:value-of select="@checked"/></span> links really checked</td></tr>
	  <tr><td class="label" width="50%">Goods :</td><td width="50%"><span><xsl:value-of select="@good"/></span></td></tr>
	  <tr><td class="label">Bads :</td><td><span><xsl:value-of select="@bad"/></span> (Both not found and restricted)</td></tr>
	  <tr><td class="label">Timeouts :</td><td><span><xsl:value-of select="@timeout"/></span></td></tr>
	  <tr><td class="label">Malformed :</td><td><span><xsl:value-of select="@malformed"/></span></td></tr>
	  <tr><td class="label">Ignored :</td><td><span><xsl:value-of select="@ignored"/></span></td></tr>
	</table>
	</p>
</xsl:template>

<xsl:template match="website">
	<!-- if we must numbered lists -->
	<xsl:choose>
	  <xsl:when test="$withnum = 1">
	    <ol>
	      <xsl:apply-templates />
	    </ol>
	  </xsl:when>
	  <xsl:otherwise>
	    <ul>
	      <xsl:apply-templates />
	    </ul>
	  </xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match="page|childs/page">
	<xsl:variable name="user_action" select="@user_action"/>
	<xsl:variable name="virname" select="@virname"/>
	<xsl:variable name="href" select="@href"/>
	<xsl:variable name="status" select="properties/header/item[@name='status']/@value"/>
	<li>
	<xsl:choose>
	<!-- good -->
	<xsl:when test="
		substring($status,1,1) = 2 or
		(
			$status = 301 or
			$status = 302 or
			$status = 303 or
			$status = 798 or
		        $status = 702
		)">
		<span title="HTTP {$status}" class="status_code good">Good</span>
	</xsl:when>
	<!-- bad -->
	<xsl:when test="
		substring($status, 1, 1) != 2 and
		substring($status, 1, 1) != 9 and
		substring($status, 1, 1) != 8 and
		$status != 301 and
		$status != 302 and
		$status != 303 and
		$status != 401 and
		$status != 408 and
		$status != 503 and
		$status != 797 and
		$status != 702
		">
		<span title="HTTP {$status}" class="status_code bad">Bad</span>
	</xsl:when>
	<!-- timeout -->
	<xsl:when test="
		(
			substring($status, 1, 1) = 9 or
			$status = 503 or
			$status = 408
		) 
		and
		(
			$status != 997 and
			$status != 899 and
			$status != 898
		)
		">
 		<span title="HTTP {$status}" class="status_code timeout">Timeout</span>
	</xsl:when>
	<!-- ignored -->
	<xsl:when test="$status = 997">
 		<span title="HTTP {$status}" class="status_code ignored">Ignored</span>
	</xsl:when>
	<!-- malformed -->
	<xsl:when test="$status = 899 or $status = 898">
 		<span title="HTTP {$status}" class="status_code malformed">Malformed</span>
	</xsl:when>
	<!-- restricted -->
	<xsl:when test="$status = 401">
 		<span title="HTTP {$status}" class="status_code restricted">Restricted</span>
	</xsl:when>
	</xsl:choose>
	<a href="{$href}" target="_BLANK"><xsl:value-of select="@href" /></a>
	<!-- if we must display links labels -->
	<xsl:if test="$withlabel = 1">
	  - 
	  <i>
	  <xsl:choose>
	    <xsl:when test="string-length(@title)>50">
              <xsl:value-of select="substring(@title,1,50)"/>[...]
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="@title"/>
	    </xsl:otherwise>
	  </xsl:choose>
	  </i>
	</xsl:if>
        <!-- if a user action has been defined -->
        <xsl:if test="$user_action != '' and $user_action != 'none'">
          <div class="user-action-section">
            <span class="todo">TODO</span> :
            <span class="action"><xsl:value-of select="$user_action"/></span>
          </div>
        </xsl:if>
        <!-- if a virus has been found -->
        <xsl:if test="$virname != ''">
          <div class="virname-section">
            <span class="warning">VIRUS WARNING</span> :
            <span class="virname"><xsl:value-of select="$user_action"/></span>
          </div>
        </xsl:if>
	</li>
	<!-- if we must numbered lists -->
	<xsl:choose>
	  <xsl:when test="$withnum = 1">
	    <ol>
	      <xsl:apply-templates select="childs/page"/>
	    </ol>
	  </xsl:when>
	  <xsl:otherwise>
	    <ul>
	      <xsl:apply-templates select="childs/page"/>
	    </ul>
	  </xsl:otherwise>
	</xsl:choose>
</xsl:template>

</xsl:stylesheet>


