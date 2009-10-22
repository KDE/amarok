<!--
########################################################################################
# Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            #
#                                                                                      #
# This program is free software; you can redistribute it and/or modify it under        #
# the terms of the GNU General Public License as published by the Free Software        #
# Foundation; either version 2 of the License, or (at your option) any later           #
# version.                                                                             #
#                                                                                      #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY      #
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      #
# PARTICULAR PURPOSE. See the GNU General Public License for more details.             #
#                                                                                      #
# You should have received a copy of the GNU General Public License along with         #
# this program.  If not, see <http://www.gnu.org/licenses/>.                           #
########################################################################################
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html"/>


<xsl:template match="TestCase">
    <html><body>
        <h1>
            Unit test results for: 
            <xsl:value-of select="@name"/>
            <xsl:apply-templates/>
        </h1>
    </body></html>
</xsl:template>


<xsl:template match="Environment">
    <p><h3>
        Qt Version: 
        <xsl:value-of select="QtVersion"/>
    </h3></p><br/>
</xsl:template>


<xsl:template match="TestFunction">
    <h2>
        Test Function Name:
        <em>
            <xsl:value-of select="@name"/>
        </em>
        <h3>

            <xsl:if test="Incident/@type='pass'">
                Pass
            </xsl:if>
            <xsl:if test="Incident/@type='fail'">
                <font color='red'>
                    FAIL at line: 
                    <xsl:value-of select="Incident/@line"/>
                    <br/><br/>
                    <xsl:value-of select="Incident/Description"/>
                    <h4><p><ul>
                        <xsl:for-each select="Message">
                            <li>
                                File:
                                <xsl:value-of select="@file"/>
                                <br/>
                                Line:
                                <xsl:value-of select="@line"/>
                                <br/><br/>
                            </li>
                        </xsl:for-each>
                    </ul></p></h4>
                </font>
            </xsl:if>
        </h3>
    </h2>
</xsl:template>


</xsl:stylesheet>


