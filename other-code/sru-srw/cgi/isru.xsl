<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output version="1.0" encoding="UTF-8" indent="no" omit-xml-declaration="no" media-type="text/html" />
    <xsl:template match="/">
        <html>
            <head>
                <title>A/WWW Enterprises SRW Databases</title>
            </head>
            <body title="A/WWW Enterprises SRW Databases">
                <xsl:for-each select="isru">
                    <h1>
                        <xsl:for-each select="title">
                            <xsl:apply-templates />
                        </xsl:for-each>
                    </h1>
                    <br />
                    <h2>
                        <xsl:for-each select="description">
                            <xsl:apply-templates />
                        </xsl:for-each>
                    </h2>
                    <h3>Connection Information</h3>
                    <p>
                        <span style="font-style:italic; ">Host:</span>&#160;<xsl:for-each select="host">
                            <xsl:apply-templates />
                        </xsl:for-each>
                        <br />
                        <span style="font-style:italic; ">Port:</span>&#160;<xsl:for-each select="port">
                            <xsl:apply-templates />
                        </xsl:for-each>
                        <br />Current status: <xsl:for-each select="status">
                            <xsl:apply-templates />
                        </xsl:for-each>
                        <br />
                        <p>You can retrieve the descriptive information for a database by selecting its link in the table below:<br />
                            <xsl:for-each select="dblist">
                                <table border="1">
                                    <thead>
                                        <tr>
                                            <td>
                                                <center>
                                                    <span style="font-weight:bold; ">Database</span>
                                                </center>
                                            </td>
                                            <td>
                                                <center>
                                                    <span style="font-style:italic; font-weight:bold; ">sru</span>
                                                </center>
                                            </td>
                                        </tr>
                                    </thead>
                                    <tbody>
                                        <xsl:for-each select="db">
                                            <tr>
                                                <td>
                                                    <xsl:for-each select="@name">
                                                        <a>
                                                            <xsl:attribute name="href"><xsl:value-of select="../@linkage" /></xsl:attribute>
                                                            <xsl:value-of select="." />
                                                        </a>
                                                    </xsl:for-each>
                                                </td>
                                                <td>
                                                    <xsl:for-each select="@sru">
                                                        <xsl:value-of select="." />
                                                    </xsl:for-each>
                                                </td>
                                            </tr>
                                        </xsl:for-each>
                                    </tbody>
                                </table>
                            </xsl:for-each>
                        </p>
                    </p>
                </xsl:for-each>
            </body>
        </html>
    </xsl:template>
    <xsl:template match="db">
        <xsl:apply-templates />
    </xsl:template>
    <xsl:template match="isru">
        <xsl:apply-templates />
    </xsl:template>
    <xsl:template match="status">
        <xsl:apply-templates />
    </xsl:template>
</xsl:stylesheet>
