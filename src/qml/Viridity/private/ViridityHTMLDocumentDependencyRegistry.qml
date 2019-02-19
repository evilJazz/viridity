import QtQuick 2.0

QtObject {
    id: dependencyRegistry

    property variant dependencyItems: []
    property variant includeUrls: []

    function unregister(item)
    {
        var index = dependencyItems.indexOf(item);

        if (index == -1)
            return;

        var deps = dependencyItems;
        deps.splice(index, 1);
        dependencyItems = deps;

        console.log("unregisterDependencies: " + item);
        document.invalidateContent();
    }

    function register(item)
    {
        if (dependencyItems.indexOf(item) > -1)
            return;

        var deps = dependencyItems;
        deps.push(item);
        dependencyItems = deps;

        console.log("registerDependencies: " + item);

        if (!hasIncludes(item.dependencies))
        {
            updateIncludes();
            document.invalidateContent();
        }
    }

    function hasInclude(include)
    {
        return includeUrls.indexOf(include) > -1;
    }

    function hasIncludes(includes)
    {
        for (var i = 0; i < includes.length; ++i)
        {
            if (includeUrls.indexOf(includes[i]) == -1)
                return false;
        }

        return true;
    }

    function updateIncludes()
    {
        var newIncludes = [];

        for (var i = 0; i < dependencyItems.length; ++i)
        {
            var depItem = dependencyItems[i];

            for (var j = 0; j < depItem.dependencies.length; ++j)
            {
                var include = depItem.dependencies[j];
                if (newIncludes.indexOf(include) == -1)
                {
                    newIncludes.push(include);
                }
            }
        }

        includeUrls = newIncludes;
    }

    function getHTMLSnippet()
    {
        var includeMarkup = "";

        for (var i = 0; i < includeUrls.length; ++i)
        {
            var url = makeRelative(includeUrls[i]);

            if (/.css/.test(url))
            {
                includeMarkup += '<link type="text/css" rel="stylesheet" href="' + url + '">\n';
            }
            else if (/.js/.test(url))
            {
                includeMarkup += '<script type="text/javascript" src="' + url + '"></script>\n';
            }
        }

        return includeMarkup;
    }
}
