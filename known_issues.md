<!--
	SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
	SPDX-License-Identifier: MIT
-->

# 1. Lazy HTML

In contrast to CommonMark, `md4qt` has one difference. If in list item first element is HTML
CommonMark doesn't apply rule to lazy continuation lines as this is not a paragraph,
whereas `md4qt` does so. For example.

```md
* <!--
-->
```

In CommonMark will be.

```html
<ul>
<li>
<!--
</li>
</ul>
<p>--&gt;</p>
```

And in `md4qt` it will be.

```html
<ul>
<li>
<!--
-->
</li>
</ul>
```

I know about this difference. And I decided to keep it. But if somebody
will provide compelling reasons to do exactly as in CommonMark in this case, I will
work with this question.

The same issue is in blockquotes too.

**Interested in this question may start a discussion with me [here](https://invent.kde.org/libraries/md4qt/-/issues/5).**

# 2. Content of task list item

GitHub treats everything after task list item as paragraph. I.e.

```md
* [ ] > text
```

Will be.

```html
<ul>
<li class="task-list-item"><input type="checkbox" id="" disabled=""> &gt; text</li>
</ul>
```

Whereas in `md4qt` it will be.

```html
<ul>
<li class="task-list-item"><input type="checkbox" id="" disabled="">

<blockquote><p>text</p></blockquote>
</li>
</ul>
```

Interesting question. And I decided to keep it as is now.

**Interested in this question may start a discussion with me [here](https://invent.kde.org/libraries/md4qt/-/issues/6).**

# 3. Autolinks.

In this library autolinks are checked by `QUrl` for validity. CommonMark is not so strict,
his rules are much more simpler for it. As example.

```md
<made-up-scheme://foo,bar>
```

In CommonMark will be a valid autolink, whereas in `md4qt` won't be.

I can say that I will win very much in performance if to replace `QUrl` with
simple functions in corresponding with Markdown standard, but...

**Interested in this question may start a discussion with me [here](https://invent.kde.org/libraries/md4qt/-/issues/7).**
