class String

    def shorten(limit)
    if self.length > limit
        self+". " =~ /^(.{#{limit}}[^.!;?]*[.!;?])/mi
        return $1
    end
    self
    end

    def riphtml
    self.gsub(/<[^>]+>/, '').gsub(/&amp;/,'&').gsub(/&quot;/,'"').gsub(/&lt;/,'<').gsub(/&gt;/,'>').gsub(/&ellip;/,'...').gsub(/&apos;/, "'").gsub("\n",'')
    end

    def mysqlize
    self.gsub(/'/, "''")
    end
end