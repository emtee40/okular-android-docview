/***************************************************************************
 *   Copyright (C) 2018        Intevation GmbH <intevation@intevation.de>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

/* Builtin functions for Okular's PDF JavaScript interpretation. */

/** AFSimple_Calculate
 *
 * cFunction is a string that identifies the operation.
 *           It is one of AVG, SUM, PRD, MIN, MAX
 * cFields is an array of the names of the fields used to calculate.
 */
function AFSimple_Calculate( cFunction, cFields )
{
    var ret = 0;

    if ( cFunction === "PRD" )
    {
        ret = 1;
    }

    for (i = 0; i < cFields.length; i++)
    {
        var field = Doc.getField( cFields[i] );
        var val = Number( field.value );

        if ( cFunction === "SUM" || cFunction === "AVG" )
        {
            ret += val;
        }
        else if ( cFunction === "PRD" )
        {
            ret *= val;
        }
        else if ( cFunction === "MIN" )
        {
            if ( i === 0 || val < ret )
            {
                ret = val;
            }
        }
        else if ( cFunction === "MAX" )
        {
            if ( i === 0 || val > ret )
            {
                ret = val;
            }
        }
    }

    if ( cFunction === "AVG" )
    {
        ret /= cFields.length;
    }

    event.value = ret;
}

/** AFTime_Format
 *
 * Formats event.value based on parameters.
 *
 * Parameter description based on Acrobat Help:
 *
 * ptf is the number which should be used to format the time, is one of:
 * 0 = 24HR_MM [ 14:30 ] 
 * 1 = 12HR_MM [ 2:30 PM ] 
 * 2 = 24HR_MM_SS [ 14:30:15 ] 
 * 3 = 12HR_MM_SS [ 2:30:15 PM ]
 */
function AFTime_Format( ptf )
{
    if( !event.value )
    {
        return;
    }
    var tokens = event.value.split( /\D/ );
    var invalidDate = false;

    // Remove empty elements of the array
    tokens = tokens.filter(Boolean);

    if( tokens.length < 2 )
        invalidDate = true;

    // Check if every number is valid
    for( i = 0 ; i < tokens.length ; ++i )
    {
        if( isNaN( tokens[i] ) )
        {
            invalidDate = true;
            break;
        }
        switch( i )
        {
            case 0:
            {
                if( tokens[i] > 23 || tokens[i] < 0 )
                    invalidDate = true;
                break;
            }
            case 1:
            case 2:
            {
                if( tokens[i] > 59 || tokens[i] < 0 )
                    invalidDate = true;
                break;
            }
        }
    }
    if( invalidDate )
    {
        event.value = "";
        return;
    }

    // Make it of lenght 3, since we use hh, mm, ss
    while( tokens.length < 3 )
        tokens.push( 0 );

    // We get pm string in the user locale to search.
    var dummyPm = util.printd( 'ap', new Date( 2018, 5, 11, 23, 11, 11) ).toLocaleLowerCase();
    // Add 12 to time if it's PM and less than 12
    if( event.value.toLocaleLowerCase().search( dummyPm ) !== -1 && Number( tokens[0] ) < 12 )
        tokens[0] = Number( tokens[0] ) + 12;

    // We use a random date, because we only care about time.
    var date = new Date( 2019, 7, 12, tokens[0], tokens[1], tokens[2] );
    var ret;
    switch( ptf )
    {
        case 0:
            ret = util.printd( "hh:MM", date );
            break;
        case 1:
            ret = util.printd( "h:MM ap", date );
            break;
        case 2:
            ret = util.printd( "hh:MM:ss", date );
            break;
        case 3:
            ret = util.printd( "h:MM:ss ap", date );
            break;
    }
    event.value = ret;
}

/** AFTime_Keystroke
 *
 * Checks if the string in event.value is valid. Not used.
 */
function AFTime_Keystroke( ptf )
{
    return;
}

/** AFSpecial_Format
 * psf is the type of formatting to use: 
 * 0 = zip code
 * 1 = zip + 4 
 * 2 = phone 
 * 3 = SSN
 *
 * These are all in the US format.
*/
function AFSpecial_Format( psf )
{
    if( !event.value || psf == 0 )
    {
        return;
    }

    var ret = event.value;

    if( psf === 1 )
        ret = ret.substr( 0, 5 ) + '-' + ret.substr( 5, 4 );

    else if( psf === 2 )
        ret = '(' + ret.substr( 0, 3 ) + ') ' + ret.substr( 3, 3 ) + '-' + ret.substr( 6, 4 );

    else if( psf === 3 )
        ret = ret.substr( 0, 3 ) + '-' + ret.substr( 3, 2 ) + '-' + ret.substr( 5, 4 );

    event.value = ret;
} 

/** AFSpecial_Keystroke
 *
 * Checks if the String in event.value is valid.
 *
 * Parameter description based on Acrobat Help:
 *
 * psf is the type of formatting to use: 
 * 0 = zip code
 * 1 = zip + 4 
 * 2 = phone 
 * 3 = SSN
 *
 * These are all in the US format. We check to see if only numbers are inserted and the length of the string.
*/
function AFSpecial_Keystroke( psf )
{
    if ( !event.value )
    {
        return;
    }

    var str = event.value;
    if( psf === 0 )
    {
        if( str.length > 5 )
        {
            event.rc = false;
            return;
        }
    }

    else if( psf === 1 || psf === 3 )
    {
        if( str.length > 9 )
        {
            event.rc = false;
            return;
        }
    }

    else if( psf === 2 )
    {
        if( str.length > 10 )
        {
            event.rc = false;
            return;
        }
    }

    for( i = 0 ; i < str.length ; ++i )
    {
        if( !( str[i] <= '9' && str[i] >= '0' ) )
        {
            event.rc = false;
            return;
        }
    }
}

/** AFPercent_Format
 *
 * Formats event.value based on parameters.
 *
 * Parameter description based on Acrobat Help:
 *
 * nDec is the number of places after the decimal point
 * sepStyle is an integer denoting the style of the separator, which is one of:
 * 0 - 1,234.56
 * 1 - 1234.56
 * 2 - 1.234,56
 * 3 - 1234,56 
 * 4 - 1'234.56
*/
function AFPercent_Format( nDec, sepStyle )
{
    if ( !event.value )
    {
        return;
    }

    var ret;
    var localized = Number( event.target.value );

    if( isNaN( localized ) )
    {
        event.value = 'NaN';
        return;
    }

    localized = localized * 100;

    localized = localized.toFixed( nDec );
    
    var locale = 'de-DE';
    var useGrouping = true;

    if ( sepStyle === 0 )
    {
        locale = 'en-US';
    }
    else if ( sepStyle === 1 )
    {
        locale = 'en-US'
        useGrouping = false;
    }
    else if ( sepStyle === 3 )
    {
        useGrouping = false;
    }

    var decPart = localized.substr( localized.indexOf( /\D/ )-1, nDec );
    var intPart = Math.trunc( localized );

    if( useGrouping )
    {
       intPart = intPart.toString().replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,')
    }

    if( locale === 'en-US' )
    {
        ret = intPart.toString().replace( /\./g, '.' ) + '.' + decPart;
    }
    else
    {
        ret = intPart.toString().replace( /,/g, (sepStyle === 4 ? '\'' : '.' ) ) + (sepStyle === 4 ? '.' : ',') + decPart;
    }

    ret = ret + ' %';

    event.value = ret;
} 

/** AFPercent_Keystroke
 *
 * Checks if the String in event.value is valid.
 *
 * Parameter description based on Acrobat Help:
 *
 * nDec is the number of places after the decimal point
 * sepStyle is an integer denoting the style of the separator, which is one of:
 * 0 - 1,234.56
 * 1 - 1234.56
 * 2 - 1.234,56
 * 3 - 1234,56 
 * 4 - 1'234.56
 *
 * We only support numbers written in the form of 1234,56 or 1234.56, depending on what is sepStyle.
 * If sepStyle >= 2, we use commas to separate and don't allow dots.
 * If sepStyle <= 2, we use dots to separate and don't allow commas.
*/
function AFPercent_Keystroke( nDec, sepStyle )
{
    if ( !event.value )
    {
        return;
    }

    var number = event.target.value;

   if( sepStyle >= 2 )
    {
        number = number.replace( '.', 'A' );
        number = number.replace( ',', '.' );
    }

    if( isNaN( number ) ){
        event.rc = false;
        event.value = 'NaN';
        return;
    }
    event.value = number;
}
