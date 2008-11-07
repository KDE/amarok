function Service()
{
    ScriptableServiceScript.call( this, "Template Name", 1, "Title", "Introduction", false );
}

function onConfigure()
{
    Amarok.alert( "This script does not require any configuration." );
}

function onPopulating( level, callbackData, filter )
{
    Amarok.debug( "populating level " + level );

    var numberOfItems = 10;
    
    for ( i = 0; i < numberOfItems; i++ )
    {
        item = Amarok.StreamItem;
        item.level = ;
        item.callbackData = ;
        item.itemName = ;
        item.playableUrl = ;
        item.infoHtml = ;
        script.insertItem( item );
    }
    script.donePopulating();
}

Amarok.configured.connect( onConfigure );
script = new Service();
script.populate.connect( onPopulating );